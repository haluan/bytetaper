// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_RUNTIME_WORKER_QUEUE_H
#define BYTETAPER_RUNTIME_WORKER_QUEUE_H

#include "cache/cache_entry.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>

namespace bytetaper::runtime {

// Maximum body size for an async cache job.
static constexpr std::size_t kAsyncL2MaxBodySize = 65536; // 64 KB

// Upper bounds enforced by worker_queue_init.
static constexpr std::size_t kWorkerQueueMaxWorkers = 8;
static constexpr std::size_t kWorkerQueueMaxCapacity = 1024;

enum class RuntimeJobKind : std::uint8_t {
    L2Lookup = 0,
    L2Store = 1,
};

// All fields are value-owned. No raw pointers to request context or protobuf strings.
// entry.body always points into body[] — fixed by worker_queue_try_enqueue.
struct RuntimeCacheJob {
    RuntimeJobKind kind = RuntimeJobKind::L2Lookup;
    char key[cache::kCacheKeyMaxLen] = {};
    cache::CacheEntry entry = {};
    char body[kAsyncL2MaxBodySize] = {};
    std::size_t body_len = 0;
};

struct WorkerQueueConfig {
    std::size_t capacity = 64;    // ring buffer slots; <= kWorkerQueueMaxCapacity
    std::size_t worker_count = 2; // >= 1, <= kWorkerQueueMaxWorkers
};

// Fixed-capacity worker queue. Must not be copied or moved after init.
struct WorkerQueue {
    std::mutex mu;
    std::condition_variable cv;
    RuntimeCacheJob slots[kWorkerQueueMaxCapacity] = {};
    std::size_t head = 0;
    std::size_t tail = 0;
    std::size_t count = 0;
    std::size_t capacity = 0;
    bool running = false;
    std::thread workers[kWorkerQueueMaxWorkers];
    std::size_t worker_count = 0;
    std::atomic<std::uint64_t> dropped_count{ 0 }; // promoted to RuntimeMetrics in BT-035-007
};

// Validates config and initialises queue fields. Does not start threads.
// Returns nullptr on success, error string on failure.
const char* worker_queue_init(WorkerQueue* q, const WorkerQueueConfig& config);

// Starts worker_count threads. Returns nullptr on success.
// Must be called after worker_queue_init.
const char* worker_queue_start(WorkerQueue* q);

// Non-blocking enqueue. Returns true if accepted, false if queue is full or not running.
// Copies all job data into the ring slot; fixes entry.body -> slot.body.
// Thread-safe.
bool worker_queue_try_enqueue(WorkerQueue* q, const RuntimeCacheJob& job);

// Signals workers to stop and joins all threads.
// Caller must keep any L2DiskCache alive until this returns.
void worker_queue_shutdown(WorkerQueue* q);

} // namespace bytetaper::runtime

#endif // BYTETAPER_RUNTIME_WORKER_QUEUE_H
