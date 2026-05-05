// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/cache_key.h"
#include "cache/l1_cache.h"
#include "cache/l2_disk_cache.h"
#include "metrics/runtime_metrics.h"
#include "runtime/worker_queue.h"
#include "stages/l2_cache_async_store_enqueue_stage.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>

namespace bytetaper::stages {

class L2CacheAsyncStoreEnqueueStageTest : public ::testing::Test {
protected:
    void SetUp() override {
        l1_cache = std::make_unique<cache::L1Cache>();
        cache::l1_init(l1_cache.get());

        // L2 is needed for stage to run, but we don't need real RocksDB for these logic tests
        l2_cache = reinterpret_cast<cache::L2DiskCache*>(0x1234);

        runtime::WorkerQueueConfig wq_config{};
        wq_config.worker_count = 1;
        runtime::worker_queue_init(&worker_queue, wq_config);
        worker_queue.running = true;

        policy.cache.behavior = policy::CacheBehavior::Store;
        policy.cache.ttl_seconds = 60;
        policy.route_id = "test_route";

        ctx.matched_policy = &policy;
        ctx.l1_cache = l1_cache.get();
        ctx.l2_cache = l2_cache;
        ctx.worker_queue = &worker_queue;
        ctx.runtime_metrics = &metrics;
        std::strcpy(ctx.raw_path, "/path");
        ctx.request_method = policy::HttpMethod::Get;
        ctx.response_status_code = 200;
        ctx.response_body = "hello";
        ctx.response_body_len = 5;
    }

    std::unique_ptr<cache::L1Cache> l1_cache;
    cache::L2DiskCache* l2_cache;
    runtime::WorkerQueue worker_queue;
    metrics::RuntimeMetrics metrics{};
    policy::RoutePolicy policy;
    apg::ApgTransformContext ctx;
};

TEST_F(L2CacheAsyncStoreEnqueueStageTest, EligibleResponseEnqueuesL2Store) {
    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "enqueued");
    std::size_t total_count = 0;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        total_count += worker_queue.shards[i].count;
    }
    EXPECT_EQ(total_count, 1u);
    EXPECT_EQ(metrics.l2_async_store_total.load(), 1);

    // Verify job content in whichever shard it landed
    bool found = false;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        if (worker_queue.shards[i].count > 0) {
            auto& slot = worker_queue.shards[i].slots[worker_queue.shards[i].head];
            EXPECT_EQ(slot.kind, runtime::RuntimeJobKind::L2Store);
            EXPECT_STREQ(slot.body, "hello");
            EXPECT_EQ(slot.body_len, 5u);
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(L2CacheAsyncStoreEnqueueStageTest, L2StoreQueueFullDoesNotFailResponse) {
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

    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "queue-full");
    EXPECT_EQ(metrics.l2_async_store_dropped_total.load(), 1);
}

TEST_F(L2CacheAsyncStoreEnqueueStageTest, L2StoreJobOwnsBodyMemory) {
    char mutable_body[] = "world";
    ctx.response_body = mutable_body;
    ctx.response_body_len = 5;

    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);

    // Overwrite original buffer
    std::memset(mutable_body, 0, 5);

    // Verify slot body is unchanged in whichever shard it landed
    bool found = false;
    for (std::size_t i = 0; i < runtime::kRuntimeShardCount; ++i) {
        if (worker_queue.shards[i].count > 0) {
            auto& slot = worker_queue.shards[i].slots[worker_queue.shards[i].head];
            EXPECT_STREQ(slot.body, "world");
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(L2CacheAsyncStoreEnqueueStageTest, OversizedBodySkipsAsyncL2Store) {
    ctx.response_body_len = runtime::kAsyncL2MaxBodySize + 1;
    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "body-too-large");
    EXPECT_EQ(metrics.l2_async_store_oversized_skipped_total.load(), 1);
}

TEST_F(L2CacheAsyncStoreEnqueueStageTest, NonGetRequestSkips) {
    ctx.request_method = policy::HttpMethod::Post;
    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "non-get");
}

TEST_F(L2CacheAsyncStoreEnqueueStageTest, Non2xxStatusSkips) {
    ctx.response_status_code = 500;
    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "non-2xx");
}

TEST_F(L2CacheAsyncStoreEnqueueStageTest, NoTtlSkips) {
    policy.cache.ttl_seconds = 0;
    auto output = l2_cache_async_store_enqueue_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
    EXPECT_STREQ(output.note, "no-ttl");
}

} // namespace bytetaper::stages
