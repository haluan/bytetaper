// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/cache_key.h"
#include "cache/l1_cache.h"
#include "cache/l2_disk_cache.h"
#include "metrics/runtime_metrics.h"
#include "runtime/pending_lookup_registry.h"
#include "runtime/worker_queue.h"
#include "stages/l2_cache_async_lookup_enqueue_stage.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>

namespace bytetaper::stages {

class L2CacheAsyncLookupEnqueueStageTest : public ::testing::Test {
protected:
    void SetUp() override {
        l1_cache = std::make_unique<cache::L1Cache>();
        cache::l1_init(l1_cache.get());

        // L2 is needed for stage to run, but we don't need real RocksDB for these logic tests
        // unless we call l2_get. Here we just need the pointer.
        l2_cache = reinterpret_cast<cache::L2DiskCache*>(0x1234);

        pending_lookup_init(&pending_registry);

        runtime::WorkerQueueConfig wq_config{};
        wq_config.capacity = 2;
        wq_config.worker_count = 1;
        runtime::worker_queue_init(&worker_queue, wq_config);
        // We don't start the queue threads to avoid background complexity in unit tests.
        // We just manually set running=true so try_enqueue works.
        worker_queue.running = true;

        policy.cache.behavior = policy::CacheBehavior::Store;
        policy.route_id = "test_route";

        ctx.matched_policy = &policy;
        ctx.l1_cache = l1_cache.get();
        ctx.l2_cache = l2_cache;
        ctx.worker_queue = &worker_queue;
        ctx.pending_lookup_registry = &pending_registry;
        ctx.runtime_metrics = &metrics;
        std::strcpy(ctx.raw_path, "/path");
        ctx.request_method = policy::HttpMethod::Get;
    }

    std::unique_ptr<cache::L1Cache> l1_cache;
    cache::L2DiskCache* l2_cache;
    runtime::PendingLookupRegistry pending_registry;
    runtime::WorkerQueue worker_queue;
    metrics::RuntimeMetrics metrics{};
    policy::RoutePolicy policy;
    apg::ApgTransformContext ctx;
};

TEST_F(L2CacheAsyncLookupEnqueueStageTest, L1HitSkipsEnqueue) {
    ctx.cache_hit = true;
    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "l1-hit-skip");
    EXPECT_EQ(worker_queue.count, 0);
}

TEST_F(L2CacheAsyncLookupEnqueueStageTest, L1MissEnqueuesAndContinues) {
    ctx.cache_hit = false;
    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "enqueued");
    EXPECT_EQ(worker_queue.count, 1);
    EXPECT_EQ(pending_registry.count, 1);
    EXPECT_EQ(metrics.l2_async_lookup_total.load(), 1);
}

TEST_F(L2CacheAsyncLookupEnqueueStageTest, DuplicateKeySkipped) {
    ctx.cache_hit = false;
    // Pre-mark key
    char key[cache::kCacheKeyMaxLen];
    cache::CacheKeyInput ki{};
    ki.method = ctx.request_method;
    ki.route_id = policy.route_id;
    ki.path = ctx.raw_path;
    ki.policy_version = policy.route_id;
    cache::build_cache_key(ki, key, sizeof(key));
    runtime::pending_lookup_try_mark(&pending_registry, key);

    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "already-pending");
    EXPECT_EQ(worker_queue.count, 0);
    EXPECT_EQ(metrics.l2_async_lookup_deduped_total.load(), 1);
}

TEST_F(L2CacheAsyncLookupEnqueueStageTest, QueueFullContinuesPendingCleared) {
    ctx.cache_hit = false;
    // Fill queue
    runtime::RuntimeCacheJob job{};
    runtime::worker_queue_try_enqueue(&worker_queue, job);
    runtime::worker_queue_try_enqueue(&worker_queue, job);
    EXPECT_EQ(worker_queue.count, 2);

    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "queue-full");
    EXPECT_EQ(pending_registry.count, 0); // Must be cleared
}

} // namespace bytetaper::stages
