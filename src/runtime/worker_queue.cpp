// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "runtime/worker_queue.h"

#include "cache/l1_cache.h"
#include "cache/l2_disk_cache.h"
#include "metrics/runtime_metrics.h"

#include <chrono>
#include <cstring>

namespace bytetaper::runtime {

namespace {

// Simple DJB2 hash for selecting shard.
static std::uint32_t hash_key_to_shard(const char* key) {
    if (key == nullptr)
        return 0;
    std::uint32_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + static_cast<std::uint32_t>(c);
    }
    return hash % kRuntimeShardCount;
}

static bool shard_pending_try_mark(RuntimeShard* s, const char* key) {
    for (std::size_t i = 0; i < kRuntimePendingSlotsPerShard; i++) {
        if (s->pending_occupied[i] && std::strcmp(s->pending_keys[i], key) == 0) {
            return false; // duplicate
        }
    }
    for (std::size_t i = 0; i < kRuntimePendingSlotsPerShard; i++) {
        if (!s->pending_occupied[i]) {
            std::strncpy(s->pending_keys[i], key, cache::kCacheKeyMaxLen - 1);
            s->pending_occupied[i] = true;
            s->pending_count++;
            return true;
        }
    }
    return false; // full
}

static void shard_pending_clear(RuntimeShard* s, const char* key) {
    for (std::size_t i = 0; i < kRuntimePendingSlotsPerShard; i++) {
        if (s->pending_occupied[i] && std::strcmp(s->pending_keys[i], key) == 0) {
            s->pending_occupied[i] = false;
            s->pending_count--;
            return;
        }
    }
}

static void execute_job_internal(WorkerQueue* q, RuntimeShard* shard, RuntimeCacheJob& job) {
    ::bytetaper::metrics::record_runtime_event(
        q->resources.runtime_metrics, ::bytetaper::metrics::RuntimeMetricEvent::JobExecuted);

    // Dispatch based on job kind
    if (job.kind == RuntimeJobKind::L2Lookup) {
        auto* l1 = q->resources.l1_cache;
        auto* l2 = q->resources.l2_cache;
        auto* m = q->resources.runtime_metrics;

        if (l2 != nullptr) {
            cache::CacheEntry hit{};
            const std::int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::system_clock::now().time_since_epoch())
                                            .count();

            // Note: job.body in the slot is where we read the L2 body into.
            bool found =
                cache::l2_get(l2, job.entry.key, now_ms, &hit, job.body, kAsyncL2MaxBodySize);
            if (found) {
                ::bytetaper::metrics::record_runtime_event(
                    m, ::bytetaper::metrics::RuntimeMetricEvent::L2LookupHit);
                if (l1 != nullptr) {
                    if (cache::l1_put_if_newer(l1, hit)) {
                        ::bytetaper::metrics::record_runtime_event(
                            m, ::bytetaper::metrics::RuntimeMetricEvent::L2ToL1Promotion);
                    } else {
                        ::bytetaper::metrics::record_runtime_event(
                            m, ::bytetaper::metrics::RuntimeMetricEvent::L2ToL1StaleRejected);
                    }
                }
            } else {
                ::bytetaper::metrics::record_runtime_event(
                    m, ::bytetaper::metrics::RuntimeMetricEvent::L2LookupMiss);
            }
        } else {
            ::bytetaper::metrics::record_runtime_event(
                m, ::bytetaper::metrics::RuntimeMetricEvent::L2LookupError);
            ::bytetaper::metrics::record_runtime_event(
                m, ::bytetaper::metrics::RuntimeMetricEvent::JobError);
        }

        {
            std::lock_guard<std::mutex> lock(shard->mu);
            shard_pending_clear(shard, job.entry.key);
        }
    } else if (job.kind == RuntimeJobKind::L2Store) {
        auto* l2 = q->resources.l2_cache;
        auto* m = q->resources.runtime_metrics;
        if (l2 != nullptr) {
            // entry.body already points to job.body (fixed in dequeue logic)
            if (cache::l2_put(l2, job.entry)) {
                ::bytetaper::metrics::record_runtime_event(
                    m, ::bytetaper::metrics::RuntimeMetricEvent::L2StoreSuccess);
            } else {
                ::bytetaper::metrics::record_runtime_event(
                    m, ::bytetaper::metrics::RuntimeMetricEvent::L2StoreError);
                ::bytetaper::metrics::record_runtime_event(
                    m, ::bytetaper::metrics::RuntimeMetricEvent::JobError);
            }
        } else {
            ::bytetaper::metrics::record_runtime_event(
                m, ::bytetaper::metrics::RuntimeMetricEvent::L2StoreError);
            ::bytetaper::metrics::record_runtime_event(
                m, ::bytetaper::metrics::RuntimeMetricEvent::JobError);
        }
    }
}

static void worker_loop(WorkerQueue* q, std::size_t worker_id) {
    while (true) {
        bool found_work = false;
        RuntimeCacheJob job;
        RuntimeShard* selected_shard = nullptr;

        // Round-robin or fixed-assignment across shards owned by this worker.
        // Worker i owns shards where shard_id % worker_count == i.
        for (std::size_t s_idx = 0; s_idx < kRuntimeShardCount; ++s_idx) {
            if (s_idx % q->worker_count != worker_id) {
                continue;
            }

            RuntimeShard& shard = q->shards[s_idx];
            std::unique_lock<std::mutex> lock(shard.mu, std::try_to_lock);
            if (!lock.owns_lock()) {
                continue;
            }

            if (shard.count > 0) {
                job = shard.slots[shard.head];
                job.entry.body = job.body; // Fix pointer in local copy

                shard.head = (shard.head + 1) % kRuntimeQueueSlotsPerShard;
                shard.count--;
                found_work = true;
                selected_shard = &shard;

                if (q->resources.runtime_metrics != nullptr) {
                    q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                        1, std::memory_order_relaxed);
                }
                break; // Process this job
            }
        }

        if (found_work) {
            execute_job_internal(q, selected_shard, job);
            continue;
        }

        // No work found in any owned shard. Wait on the first owned shard's CV as a proxy.
        std::size_t first_shard_idx = worker_id % kRuntimeShardCount;
        RuntimeShard& primary = q->shards[first_shard_idx];
        std::unique_lock<std::mutex> lock(primary.mu);
        primary.cv.wait_for(lock, std::chrono::milliseconds(10),
                            [q, &primary] { return primary.count > 0 || !q->running; });

        if (!q->running) {
            // Check if ALL shards assigned to this worker are empty before exiting.
            bool all_empty = true;
            for (std::size_t s_idx = 0; s_idx < kRuntimeShardCount; ++s_idx) {
                if (s_idx % q->worker_count == worker_id) {
                    if (q->shards[s_idx].count > 0) {
                        all_empty = false;
                        break;
                    }
                }
            }
            if (all_empty) {
                return;
            }
        }
    }
}

} // namespace

const char* worker_queue_init(WorkerQueue* q, const WorkerQueueConfig& config) {
    if (q == nullptr) {
        return "queue pointer is null";
    }

    if (config.worker_count == 0 || config.worker_count > kWorkerQueueMaxWorkers) {
        return "invalid worker_count";
    }

    q->worker_count = config.worker_count;
    q->running = false;

    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        q->shards[i].head = 0;
        q->shards[i].tail = 0;
        q->shards[i].count = 0;
        q->shards[i].pending_count = 0;
        for (std::size_t j = 0; j < kRuntimePendingSlotsPerShard; j++) {
            q->shards[i].pending_occupied[j] = false;
        }
    }

    return nullptr;
}

const char* worker_queue_start(WorkerQueue* q, const WorkerQueueResources& res) {
    if (q == nullptr) {
        return "queue pointer is null";
    }

    if (q->running) {
        return "queue already running";
    }

    q->running = true;
    q->resources = res;
    if (res.runtime_metrics != nullptr) {
        res.runtime_metrics->worker_queue_capacity.store(
            kRuntimeShardCount * kRuntimeQueueSlotsPerShard, std::memory_order_relaxed);
    }
    for (std::size_t i = 0; i < q->worker_count; ++i) {
        q->workers[i] = std::thread(worker_loop, q, i);
    }

    return nullptr;
}

bool worker_queue_try_enqueue(WorkerQueue* q, const RuntimeCacheJob& job) {
    if (q == nullptr || !q->running) {
        return false;
    }

    std::uint32_t shard_idx = hash_key_to_shard(job.entry.key);
    RuntimeShard& shard = q->shards[shard_idx];

    // Check pending registry first (already sharded)
    {
        std::lock_guard<std::mutex> lock(shard.mu);
        if (job.kind == RuntimeJobKind::L2Lookup) {
            if (!shard_pending_try_mark(&shard, job.entry.key)) {
                return false; // Already pending or registry full
            }
        }

        if (shard.count >= kRuntimeQueueSlotsPerShard) {
            if (job.kind == RuntimeJobKind::L2Lookup) {
                shard_pending_clear(&shard, job.entry.key);
            }
            ::bytetaper::metrics::record_runtime_event(
                q->resources.runtime_metrics,
                ::bytetaper::metrics::RuntimeMetricEvent::EnqueueDropped);
            return false;
        }

        ::bytetaper::metrics::record_runtime_event(
            q->resources.runtime_metrics, ::bytetaper::metrics::RuntimeMetricEvent::Enqueue);
        if (q->resources.runtime_metrics != nullptr) {
            q->resources.runtime_metrics->worker_queue_depth.fetch_add(1,
                                                                       std::memory_order_relaxed);
        }

        shard.slots[shard.tail] = job;
        shard.slots[shard.tail].entry.body = shard.slots[shard.tail].body;

        if (job.entry.body != nullptr && job.body_len > 0) {
            std::size_t copy_len =
                (job.body_len > kAsyncL2MaxBodySize) ? kAsyncL2MaxBodySize : job.body_len;
            std::memcpy(shard.slots[shard.tail].body, job.entry.body, copy_len);
            shard.slots[shard.tail].body_len = copy_len;
            shard.slots[shard.tail].entry.body_len = copy_len;
        }

        shard.tail = (shard.tail + 1) % kRuntimeQueueSlotsPerShard;
        shard.count++;
        shard.cv.notify_one();
    }

    return true;
}

void worker_queue_shutdown(WorkerQueue* q) {
    if (q == nullptr) {
        return;
    }

    q->running = false;

    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        q->shards[i].cv.notify_all();
    }

    for (std::size_t i = 0; i < q->worker_count; ++i) {
        if (q->workers[i].joinable()) {
            q->workers[i].join();
        }
    }
}

bool worker_queue_execute_one_for_test(WorkerQueue* q) {
    if (q == nullptr) {
        return false;
    }

    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        RuntimeShard& shard = q->shards[i];
        std::unique_lock<std::mutex> lock(shard.mu);
        if (shard.count > 0) {
            RuntimeCacheJob job = shard.slots[shard.head];
            job.entry.body = job.body;

            shard.head = (shard.head + 1) % kRuntimeQueueSlotsPerShard;
            shard.count--;

            if (q->resources.runtime_metrics != nullptr) {
                q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                    1, std::memory_order_relaxed);
            }

            lock.unlock(); // Release lock before I/O
            execute_job_internal(q, &shard, job);
            return true;
        }
    }

    return false;
}

} // namespace bytetaper::runtime
