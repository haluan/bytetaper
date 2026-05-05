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

    L2StoreJob job;
    std::strcpy(job.key, "test-key");
    job.entry.body = "hello";
    job.body_len = 5;

    EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), job));

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

    L2StoreJob job;
    std::strcpy(job.key, "test-key");
    job.entry.body = "data";
    job.body_len = 4;

    // shard capacity is kRuntimeQueueSlotsPerShard (4)
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), job)) << "Failed at index " << i;
    }

    EXPECT_FALSE(worker_queue_try_enqueue_store(q_.get(), job));
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

    L2StoreJob job;
    EXPECT_FALSE(worker_queue_try_enqueue_store(q_.get(), job));
}

TEST_F(WorkerQueueTest, BodyPointerFixedInSlot) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    {
        q_->running = true;
    }

    L2StoreJob job;
    const char* original_body = "hello world";
    job.entry.body = original_body;
    job.body_len = std::strlen(original_body);
    std::strcpy(job.key, "key1");

    EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), job));

    // Find which shard it went to
    std::size_t found_shard = 9999;
    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        if (q_->shards[i].store_count > 0) {
            found_shard = i;
            break;
        }
    }
    ASSERT_NE(found_shard, 9999u);

    // Check that slot entry body points inside shard body pool
    std::uint32_t slot_idx = q_->shards[found_shard].store_slots[0].body_slot;
    EXPECT_EQ(q_->shards[found_shard].store_slots[0].entry.body,
              q_->shards[found_shard].body_pool.bodies[slot_idx]);
    EXPECT_NE(q_->shards[found_shard].store_slots[0].entry.body, original_body);
    EXPECT_STREQ(q_->shards[found_shard].body_pool.bodies[slot_idx], original_body);
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

    L2StoreJob job;
    std::strcpy(job.key, "stable-key");

    std::uint32_t first_shard = 9999;
    for (int i = 0; i < 3; i++) {
        worker_queue_try_enqueue_store(q_.get(), job);
        std::uint32_t current_shard = 9999;
        for (std::uint32_t s = 0; s < kRuntimeShardCount; s++) {
            if (q_->shards[s].store_count > 0) {
                current_shard = s;
                q_->shards[s].store_count = 0; // reset
                q_->shards[s].store_head = 0;
                q_->shards[s].store_tail = 0;
                q_->shards[s].body_pool.occupied[0] = false;
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

    L2LookupJob job;
    std::strcpy(job.key, "dedupe-key");

    EXPECT_TRUE(worker_queue_try_enqueue_lookup(q_.get(), job));
    // Second enqueue for same key (while first is still in queue) should fail
    EXPECT_FALSE(worker_queue_try_enqueue_lookup(q_.get(), job));
}

TEST_F(WorkerQueueTest, RuntimeQueueShardFullDoesNotAffectOtherShard) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    worker_queue_init(q_.get(), cfg);
    q_->running = true;

    L2StoreJob jobA;
    std::strcpy(jobA.key, "key-shard-a");
    L2StoreJob jobB;
    std::strcpy(jobB.key, "key-shard-b");

    // Verify they are different
    std::uint32_t shardA = 0, shardB = 0;
    EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), jobA));
    for (std::uint32_t i = 0; i < kRuntimeShardCount; i++) {
        if (q_->shards[i].store_count > 0) {
            shardA = i;
        }
    }
    q_->shards[shardA].store_count = 0;
    q_->shards[shardA].store_tail = 0;
    q_->shards[shardA].store_head = 0;
    q_->shards[shardA].body_pool.occupied[0] = false;

    EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), jobB));
    for (std::uint32_t i = 0; i < kRuntimeShardCount; i++) {
        if (q_->shards[i].store_count > 0) {
            shardB = i;
        }
    }
    q_->shards[shardB].store_count = 0;
    q_->shards[shardB].store_tail = 0;
    q_->shards[shardB].store_head = 0;
    q_->shards[shardB].body_pool.occupied[0] = false;

    if (shardA == shardB) {
        std::strcpy(jobB.key, "another-key");
        worker_queue_try_enqueue_store(q_.get(), jobB);
        for (std::uint32_t i = 0; i < kRuntimeShardCount; i++) {
            if (q_->shards[i].store_count > 0) {
                shardB = i;
            }
        }
        q_->shards[shardB].store_count = 0;
        q_->shards[shardB].body_pool.occupied[0] = false;
    }
    ASSERT_NE(shardA, shardB);

    // Fill shard A
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), jobA));
    }
    EXPECT_FALSE(worker_queue_try_enqueue_store(q_.get(), jobA)); // Shard A full

    // Shard B should still accept
    EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), jobB));
}

TEST_F(WorkerQueueTest, LookupJobDoesNotTouchBodyPool) {
    // Assert structural memory budget constraints
    EXPECT_LT(sizeof(L2LookupJob), 1024u);
    EXPECT_LT(sizeof(L2StoreJob), 2048u);

    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    q_->running = true;

    L2LookupJob job;
    std::strcpy(job.key, "lookup-test-key");

    EXPECT_TRUE(worker_queue_try_enqueue_lookup(q_.get(), job));

    // Find shard
    std::size_t shard_idx = 9999;
    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        if (q_->shards[i].lookup_count > 0) {
            shard_idx = i;
            break;
        }
    }
    ASSERT_NE(shard_idx, 9999u);

    // Assert that the body pool in this shard is entirely unoccupied
    for (std::size_t i = 0; i < kRuntimeQueueSlotsPerShard; ++i) {
        EXPECT_FALSE(q_->shards[shard_idx].body_pool.occupied[i]);
    }
}

TEST_F(WorkerQueueTest, MixedLookupAndStoreBothEnqueue) {
    WorkerQueueConfig cfg;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    q_->running = true;

    L2LookupJob lookup_job;
    std::strcpy(lookup_job.key, "mixed-key");

    L2StoreJob store_job;
    std::strcpy(store_job.key, "mixed-key");
    store_job.entry.body = "data";
    store_job.body_len = 4;

    // Both can be enqueued to the same shard's independent rings
    EXPECT_TRUE(worker_queue_try_enqueue_lookup(q_.get(), lookup_job));
    EXPECT_TRUE(worker_queue_try_enqueue_store(q_.get(), store_job));

    // Find shard
    std::size_t shard_idx = 9999;
    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        if (q_->shards[i].lookup_count > 0) {
            shard_idx = i;
            break;
        }
    }
    ASSERT_NE(shard_idx, 9999u);

    EXPECT_EQ(q_->shards[shard_idx].lookup_count, 1u);
    EXPECT_EQ(q_->shards[shard_idx].store_count, 1u);
    EXPECT_TRUE(
        q_->shards[shard_idx].body_pool.occupied[q_->shards[shard_idx].store_slots[0].body_slot]);
}
