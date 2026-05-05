// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/runtime_metrics.h"
#include "runtime/worker_queue.h"

#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

using namespace bytetaper::runtime;

class WorkerQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        q_ = std::make_unique<WorkerQueue>();
    }

    std::unique_ptr<WorkerQueue> q_;
    bytetaper::metrics::RuntimeMetrics metrics_{};
};

TEST_F(WorkerQueueTest, InitAndStartStop) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 2;

    const char* err = worker_queue_init(q_.get(), cfg);
    ASSERT_EQ(err, nullptr) << "Init failed: " << (err ? err : "");
    WorkerQueueResources res{};
    res.runtime_metrics = &metrics_;
    EXPECT_EQ(worker_queue_start(q_.get(), res), nullptr);

    worker_queue_shutdown(q_.get());
}

TEST_F(WorkerQueueTest, EnqueueSucceeds) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    WorkerQueueResources res{};
    res.runtime_metrics = &metrics_;
    EXPECT_EQ(worker_queue_start(q_.get(), res), nullptr);

    RuntimeCacheJob job;
    job.kind = RuntimeJobKind::L2Store;
    std::strcpy(job.entry.key, "test-key");

    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), job));

    worker_queue_shutdown(q_.get());
}

TEST_F(WorkerQueueTest, QueueFullReturnsFalse) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    const char* err = worker_queue_init(q_.get(), cfg);
    ASSERT_EQ(err, nullptr) << "Init failed: " << (err ? err : "");

    {
        q_->running = true;
        q_->resources.runtime_metrics = &metrics_;
    }

    RuntimeCacheJob job;
    job.kind = RuntimeJobKind::L2Store;
    std::strcpy(job.entry.key, "test-key");
    // shard capacity is kRuntimeQueueSlotsPerShard (4)
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), job)) << "Failed at index " << i;
    }

    EXPECT_FALSE(worker_queue_try_enqueue(q_.get(), job));
    EXPECT_EQ(metrics_.worker_enqueue_dropped_total.load(), 1u);
}

TEST_F(WorkerQueueTest, EnqueueAfterShutdownReturnsFalse) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    WorkerQueueResources res{};
    res.runtime_metrics = &metrics_;
    EXPECT_EQ(worker_queue_start(q_.get(), res), nullptr);
    worker_queue_shutdown(q_.get());

    RuntimeCacheJob job;
    EXPECT_FALSE(worker_queue_try_enqueue(q_.get(), job));
}

TEST_F(WorkerQueueTest, BodyPointerFixedInSlot) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    {
        q_->running = true;
    }

    RuntimeCacheJob job;
    const char* original_body = "hello world";
    job.entry.body = original_body;
    job.body_len = std::strlen(original_body);
    std::strcpy(job.entry.key, "key1");

    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), job));

    // Find which shard it went to
    std::size_t found_shard = 9999;
    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        if (q_->shards[i].count > 0) {
            found_shard = i;
            break;
        }
    }
    ASSERT_NE(found_shard, 9999u);

    // Check that the slot has a pointer to its OWN body buffer, not the original_body
    EXPECT_EQ(q_->shards[found_shard].slots[0].entry.body, q_->shards[found_shard].slots[0].body);
    EXPECT_NE(q_->shards[found_shard].slots[0].entry.body, original_body);
    EXPECT_STREQ(q_->shards[found_shard].slots[0].body, original_body);
}

TEST_F(WorkerQueueTest, InitInvalidWorkerCountZero) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 0;
    EXPECT_STREQ(worker_queue_init(q_.get(), cfg), "invalid worker_count");
}

TEST_F(WorkerQueueTest, InitWorkerCountExceedsMax) {
    WorkerQueueConfig cfg;
    cfg.worker_count = kWorkerQueueMaxWorkers + 1;
    EXPECT_STREQ(worker_queue_init(q_.get(), cfg), "invalid worker_count");
}

TEST_F(WorkerQueueTest, RuntimeQueueMapsKeyToStableShard) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    worker_queue_init(q_.get(), cfg);
    q_->running = true;

    RuntimeCacheJob job;
    std::strcpy(job.entry.key, "stable-key");

    // We can't easily predict the hash, but it must be stable across multiple tries
    // (if it wasn't, multiple jobs for the same key would go to different shards).
    std::uint32_t first_shard = 9999;
    for (int i = 0; i < 3; i++) {
        // Find which shard a key maps to by looking at count after enqueue
        // (but we have to clear it each time or it becomes full)
        worker_queue_try_enqueue(q_.get(), job);
        std::uint32_t current_shard = 9999;
        for (std::uint32_t s = 0; s < kRuntimeShardCount; s++) {
            if (q_->shards[s].count > 0) {
                current_shard = s;
                q_->shards[s].count = 0; // reset
                q_->shards[s].head = 0;
                q_->shards[s].tail = 0;
                q_->shards[s].pending_count = 0;
                for (int p = 0; p < kRuntimePendingSlotsPerShard; p++)
                    q_->shards[s].pending_occupied[p] = false;
                break;
            }
        }
        if (i == 0)
            first_shard = current_shard;
        else
            EXPECT_EQ(first_shard, current_shard);
    }
}

TEST_F(WorkerQueueTest, RuntimeQueueSameKeyDedupesPendingLookup) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    worker_queue_init(q_.get(), cfg);
    q_->running = true;

    RuntimeCacheJob job;
    job.kind = RuntimeJobKind::L2Lookup;
    std::strcpy(job.entry.key, "dedupe-key");

    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), job));
    // Second enqueue for same key (while first is still in queue) should fail
    EXPECT_FALSE(worker_queue_try_enqueue(q_.get(), job));
}

TEST_F(WorkerQueueTest, RuntimeQueueShardFullDoesNotAffectOtherShard) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    worker_queue_init(q_.get(), cfg);
    q_->running = true;

    // We need two keys that map to different shards.
    // "key-shard-a" and "key-shard-b" are likely different.
    RuntimeCacheJob jobA;
    jobA.kind = RuntimeJobKind::L2Store;
    std::strcpy(jobA.entry.key, "key-shard-a");
    RuntimeCacheJob jobB;
    jobB.kind = RuntimeJobKind::L2Store;
    std::strcpy(jobB.entry.key, "key-shard-b");

    // Verify they are different
    std::uint32_t shardA = 0, shardB = 0;
    // (Simplest way to find shard is just to enqueue and check)
    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), jobA));
    for (std::uint32_t i = 0; i < kRuntimeShardCount; i++)
        if (q_->shards[i].count > 0)
            shardA = i;
    q_->shards[shardA].count = 0;
    q_->shards[shardA].pending_count = 0;
    q_->shards[shardA].tail = 0;
    q_->shards[shardA].head = 0;

    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), jobB));
    for (std::uint32_t i = 0; i < kRuntimeShardCount; i++)
        if (q_->shards[i].count > 0)
            shardB = i;
    q_->shards[shardB].count = 0;
    q_->shards[shardB].pending_count = 0;
    q_->shards[shardB].tail = 0;
    q_->shards[shardB].head = 0;

    if (shardA == shardB) {
        // Extremely unlikely with 256 shards, but let's be safe
        std::strcpy(jobB.entry.key, "another-key");
        worker_queue_try_enqueue(q_.get(), jobB);
        for (std::uint32_t i = 0; i < kRuntimeShardCount; i++)
            if (q_->shards[i].count > 0)
                shardB = i;
        q_->shards[shardB].count = 0;
        q_->shards[shardB].pending_count = 0;
    }
    ASSERT_NE(shardA, shardB);

    // Fill shard A
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), jobA));
    }
    EXPECT_FALSE(worker_queue_try_enqueue(q_.get(), jobA)); // Shard A full

    // Shard B should still accept
    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), jobB));
}
