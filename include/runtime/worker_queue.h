// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_RUNTIME_WORKER_QUEUE_H
#define BYTETAPER_RUNTIME_WORKER_QUEUE_H

#include "cache/cache_entry.h"
#include "cache/l2_disk_cache.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>

namespace bytetaper::cache {
struct L1Cache;
}

namespace bytetaper::metrics {
struct RuntimeMetrics;
}

namespace bytetaper::runtime {
static constexpr std::size_t kAsyncL2MaxBodySize = 65536; // 64 KB

// Sharding configuration.
static constexpr std::size_t kRuntimeShardCount = 256;
static constexpr std::size_t kRuntimeQueueSlotsPerShard = 4; // 256 * 4 = 1024 total slots
static constexpr std::size_t kRuntimePendingSlotsPerShard = 16;
static constexpr std::size_t kWorkerQueueMaxWorkers = 8;

struct L2LookupJob {
    char key[cache::kCacheKeyMaxLen] = {};
};

struct L2StoreJob {
    char key[cache::kCacheKeyMaxLen] = {};
    cache::CacheEntry entry = {};
    std::uint32_t body_slot = 0;
    std::size_t body_len = 0;
};

struct StoreBodyPool {
    char bodies[kRuntimeQueueSlotsPerShard][kAsyncL2MaxBodySize] = {};
    bool occupied[kRuntimeQueueSlotsPerShard] = {};
};

struct WorkerQueueConfig {
    std::size_t worker_count = 2; // >= 1, <= kWorkerQueueMaxWorkers
};

struct WorkerQueueResources {
    cache::L2DiskCache* l2_cache = nullptr;
    cache::L1Cache* l1_cache = nullptr;
    metrics::RuntimeMetrics* runtime_metrics = nullptr;
};

// Represents a single sharded queue with its own lock and inline pending registry.
// Each worker owns multiple shards based on its index.
struct RuntimeShard {
    alignas(64) std::mutex mu;
    std::condition_variable cv;

    // Inline pending lookup dedup — shard-local, no external mutex.
    char pending_keys[kRuntimePendingSlotsPerShard][cache::kCacheKeyMaxLen] = {};
    bool pending_occupied[kRuntimePendingSlotsPerShard] = {};
    std::size_t pending_count = 0;

    // Lookup ring.
    L2LookupJob lookup_slots[kRuntimeQueueSlotsPerShard] = {};
    std::size_t lookup_head = 0;
    std::size_t lookup_tail = 0;
    std::size_t lookup_count = 0;

    // Store ring.
    L2StoreJob store_slots[kRuntimeQueueSlotsPerShard] = {};
    std::size_t store_head = 0;
    std::size_t store_tail = 0;
    std::size_t store_count = 0;

    // Shard-local store body pool.
    StoreBodyPool body_pool = {};
};

// Fixed-capacity worker queue with sharding. Must not be copied or moved after init.
struct WorkerQueue {
    RuntimeShard shards[kRuntimeShardCount];
    bool running = false;
    std::thread workers[kWorkerQueueMaxWorkers];
    std::size_t worker_count = 0;
    WorkerQueueResources resources{};
};

// Validates config and initialises queue fields. Does not start threads.
// Returns nullptr on success, error string on failure.
const char* worker_queue_init(WorkerQueue* q, const WorkerQueueConfig& config);

// Starts the background worker threads. Returns nullptr on success, or error string.
// Must be called after worker_queue_init.
const char* worker_queue_start(WorkerQueue* q, const WorkerQueueResources& res);

// Non-blocking enqueue for lookup jobs.
bool worker_queue_try_enqueue_lookup(WorkerQueue* q, const L2LookupJob& job);

// Non-blocking enqueue for store jobs.
bool worker_queue_try_enqueue_store(WorkerQueue* q, const L2StoreJob& job);

// Signals workers to stop and joins all threads.
// Caller must keep any L2DiskCache alive until this returns.
void worker_queue_shutdown(WorkerQueue* q);

/**
 * Dequeues and synchronously executes one job using q->resources.
 * Intended for deterministic unit tests only — does not start threads.
 * Returns true if a job was dequeued and executed; false if queue was empty.
 */
bool worker_queue_execute_one_for_test(WorkerQueue* q);

} // namespace bytetaper::runtime

#endif // BYTETAPER_RUNTIME_WORKER_QUEUE_H
