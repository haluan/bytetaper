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
    cfg.capacity = 10;
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
    cfg.capacity = 10;
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
    cfg.capacity = kWorkerQueueShardCount * 5;
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
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), job)) << "Failed at index " << i;
    }

    EXPECT_FALSE(worker_queue_try_enqueue(q_.get(), job));
    EXPECT_EQ(metrics_.worker_enqueue_dropped_total.load(), 1u);
}

TEST_F(WorkerQueueTest, DroppedCountAccumulates) {
    WorkerQueueConfig cfg;
    cfg.capacity = kWorkerQueueShardCount; // Each shard gets capacity 1
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    {
        q_->running = true;
        q_->resources.runtime_metrics = &metrics_;
    }

    RuntimeCacheJob job;
    job.kind = RuntimeJobKind::L2Store;
    std::strcpy(job.entry.key, "test-key");
    worker_queue_try_enqueue(q_.get(), job); // Fill it

    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(worker_queue_try_enqueue(q_.get(), job));
    }

    EXPECT_EQ(metrics_.worker_enqueue_dropped_total.load(), 10u);
}

TEST_F(WorkerQueueTest, EnqueueAfterShutdownReturnsFalse) {
    WorkerQueueConfig cfg;
    cfg.capacity = 10;
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
    cfg.capacity = 10;
    cfg.worker_count = 1;
    ASSERT_EQ(worker_queue_init(q_.get(), cfg), nullptr);
    {
        q_->running = true;
    }

    RuntimeCacheJob job;
    const char* original_body = "hello world";
    job.entry.body = original_body;
    job.body_len = std::strlen(original_body);

    EXPECT_TRUE(worker_queue_try_enqueue(q_.get(), job));

    // Find which shard it went to
    std::size_t found_shard = 999;
    for (std::size_t i = 0; i < kWorkerQueueShardCount; ++i) {
        if (q_->shards[i].count > 0) {
            found_shard = i;
            break;
        }
    }
    ASSERT_NE(found_shard, 999u);

    // Check that the slot has a pointer to its OWN body buffer, not the original_body
    EXPECT_EQ(q_->shards[found_shard].slots[0].entry.body, q_->shards[found_shard].slots[0].body);
    EXPECT_NE(q_->shards[found_shard].slots[0].entry.body, original_body);
    EXPECT_STREQ(q_->shards[found_shard].slots[0].body, original_body);
}

TEST_F(WorkerQueueTest, InitInvalidCapacityZero) {
    WorkerQueueConfig cfg;
    cfg.capacity = 0;
    EXPECT_STREQ(worker_queue_init(q_.get(), cfg), "invalid capacity");
}

TEST_F(WorkerQueueTest, InitInvalidWorkerCountZero) {
    WorkerQueueConfig cfg;
    cfg.capacity = 10;
    cfg.worker_count = 0;
    EXPECT_STREQ(worker_queue_init(q_.get(), cfg), "invalid worker_count");
}

TEST_F(WorkerQueueTest, InitCapacityExceedsMax) {
    WorkerQueueConfig cfg;
    cfg.capacity = kWorkerQueueMaxCapacity + 1;
    EXPECT_STREQ(worker_queue_init(q_.get(), cfg), "invalid capacity");
}

TEST_F(WorkerQueueTest, InitWorkerCountExceedsMax) {
    WorkerQueueConfig cfg;
    cfg.worker_count = kWorkerQueueMaxWorkers + 1;
    EXPECT_STREQ(worker_queue_init(q_.get(), cfg), "invalid worker_count");
}
