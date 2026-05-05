// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/cache_key.h"
#include "cache/l1_cache.h"
#include "cache/l2_disk_cache.h"
#include "metrics/runtime_metrics.h"
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

        l2_cache = reinterpret_cast<cache::L2DiskCache*>(0x1234);

        runtime::WorkerQueueConfig wq_config{};
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
        ctx.runtime_metrics = &metrics;
        std::strcpy(ctx.raw_path, "/path");
        ctx.request_method = policy::HttpMethod::Get;
    }

    std::unique_ptr<cache::L1Cache> l1_cache;
    cache::L2DiskCache* l2_cache;
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
    std::size_t total_count = 0;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        total_count += worker_queue.shards[i].count;
    }
    EXPECT_EQ(total_count, 0u);
}

TEST_F(L2CacheAsyncLookupEnqueueStageTest, L1MissEnqueuesAndContinues) {
    ctx.cache_hit = false;
    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "enqueued");
    std::size_t total_count = 0;
    std::size_t pending_count = 0;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        total_count += worker_queue.shards[i].count;
        pending_count += worker_queue.shards[i].pending_count;
    }
    EXPECT_EQ(total_count, 1u);
    EXPECT_EQ(pending_count, 1u);
    EXPECT_EQ(metrics.l2_async_lookup_total.load(), 1);
}

TEST_F(L2CacheAsyncLookupEnqueueStageTest, DuplicateKeySkipped) {
    ctx.cache_hit = false;
    // Build the actual key the stage will use
    char actual_key[cache::kCacheKeyMaxLen];
    cache::CacheKeyInput ki{};
    ki.method = ctx.request_method;
    ki.route_id = policy.route_id;
    ki.path = ctx.raw_path;
    ki.policy_version = policy.route_id;
    ki.selected_field_count = ctx.selected_field_count;
    ki.selected_fields = ctx.selected_fields;
    cache::build_cache_key(ki, actual_key, sizeof(actual_key));

    // Fill all shards using the same key as the stage
    runtime::RuntimeCacheJob job{};
    std::strcpy(job.entry.key, actual_key);
    for (int i = 0; i < 100; ++i) {
        runtime::worker_queue_try_enqueue(&worker_queue, job);
    }

    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "enqueue-failed");

    std::size_t total_count = 0;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        total_count += worker_queue.shards[i].count;
    }
    EXPECT_EQ(total_count, 1u);
}

TEST_F(L2CacheAsyncLookupEnqueueStageTest, QueueFullContinuesPendingCleared) {
    ctx.cache_hit = false;
    // Build the actual key the stage will use
    char actual_key[cache::kCacheKeyMaxLen];
    cache::CacheKeyInput ki{};
    ki.method = ctx.request_method;
    ki.route_id = policy.route_id;
    ki.path = ctx.raw_path;
    ki.policy_version = policy.route_id;
    cache::build_cache_key(ki, actual_key, sizeof(actual_key));

    // Fill the shard that this key maps to
    runtime::RuntimeCacheJob job{};
    job.kind = runtime::RuntimeJobKind::L2Store;
    std::strcpy(job.entry.key, actual_key);
    for (int i = 0; i < 4; ++i) {
        runtime::worker_queue_try_enqueue(&worker_queue, job);
    }

    auto output = l2_cache_async_lookup_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "enqueue-failed");

    std::size_t pending_count = 0;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        pending_count += worker_queue.shards[i].pending_count;
    }
    EXPECT_EQ(pending_count, 0u); // Must be cleared
}

} // namespace bytetaper::stages
