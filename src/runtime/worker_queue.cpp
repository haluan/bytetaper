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

static std::uint32_t hash_key(const char* key) {
    if (key == nullptr)
        return 0;
    std::uint32_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + static_cast<std::uint32_t>(c);
    }
    return hash;
}

// Simple DJB2 hash for selecting shard.
static std::uint32_t hash_key_to_shard(const char* key) {
    return hash_key(key) % kRuntimeShardCount;
}

static std::size_t shard_owner(std::size_t shard_idx, std::size_t worker_count) {
    return shard_idx % worker_count;
}

static void notify_worker_for_shard(WorkerQueue* q, std::size_t shard_idx) {
    if (q == nullptr || q->worker_count == 0) {
        return;
    }
    std::size_t owner = shard_owner(shard_idx, q->worker_count);
    if (owner >= kWorkerQueueMaxWorkers) {
        return;
    }
    WorkerWakeState& wake = q->worker_wakes[owner];
    {
        std::lock_guard<std::mutex> lock(wake.mu);
        wake.signaled = true;
    }
    wake.cv.notify_one();
}

static bool shard_pending_try_mark(RuntimeShard* s, const char* key, std::uint32_t hash) {
    for (std::size_t i = 0; i < kRuntimePendingSlotsPerShard; i++) {
        if (s->pending_occupied[i] && s->pending_hashes[i] == hash &&
            std::strcmp(s->pending_keys[i], key) == 0) {
            return false; // duplicate
        }
    }
    for (std::size_t i = 0; i < kRuntimePendingSlotsPerShard; i++) {
        if (!s->pending_occupied[i]) {
            std::strncpy(s->pending_keys[i], key, cache::kCacheKeyMaxLen - 1);
            s->pending_hashes[i] = hash;
            s->pending_occupied[i] = true;
            s->pending_count++;
            return true;
        }
    }
    return false; // full
}

static void shard_pending_clear(RuntimeShard* s, const char* key, std::uint32_t hash) {
    for (std::size_t i = 0; i < kRuntimePendingSlotsPerShard; i++) {
        if (s->pending_occupied[i] && s->pending_hashes[i] == hash &&
            std::strcmp(s->pending_keys[i], key) == 0) {
            s->pending_occupied[i] = false;
            s->pending_hashes[i] = 0;
            s->pending_count--;
            return;
        }
    }
}

static void execute_lookup_job(WorkerQueue* q, RuntimeShard* shard, L2LookupJob& job,
                               char* scratch_buf, std::size_t scratch_len) {
    ::bytetaper::metrics::record_runtime_event(
        q->resources.runtime_metrics, ::bytetaper::metrics::RuntimeMetricEvent::JobExecuted);

    auto* l1 = q->resources.l1_cache;
    auto* l2 = q->resources.l2_cache;
    auto* m = q->resources.runtime_metrics;

    if (l2 != nullptr) {
        cache::CacheEntry hit{};
        const std::int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();

        bool found = cache::l2_get(l2, job.key, now_ms, &hit, scratch_buf, scratch_len);
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
        shard_pending_clear(shard, job.key, job.key_hash);
    }
}

static void execute_store_job(WorkerQueue* q, RuntimeShard* shard, L2StoreJob& job) {
    ::bytetaper::metrics::record_runtime_event(
        q->resources.runtime_metrics, ::bytetaper::metrics::RuntimeMetricEvent::JobExecuted);

    auto* l2 = q->resources.l2_cache;
    auto* m = q->resources.runtime_metrics;
    if (l2 != nullptr) {
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

    // Free body pool slot occupancy
    {
        std::lock_guard<std::mutex> lock(shard->mu);
        if (job.body_slot < kRuntimeQueueSlotsPerShard) {
            shard->body_pool.occupied[job.body_slot] = false;
        }
    }
}

static void worker_loop(WorkerQueue* q, std::size_t worker_id) {
    const std::size_t owned_count = q->worker_owned_shard_count[worker_id];

    while (true) {
        bool found_work = false;
        L2LookupJob lookup_job;
        L2StoreJob store_job;
        bool is_lookup = false;
        RuntimeShard* selected_shard = nullptr;

        // Round-robin or fixed-assignment across shards owned by this worker.
        for (std::size_t i = 0; i < owned_count; ++i) {
            const std::size_t s_idx = q->worker_owned_shards[worker_id][i];
            RuntimeShard& shard = q->shards[s_idx];
            std::unique_lock<std::mutex> lock(shard.mu, std::try_to_lock);
            if (!lock.owns_lock()) {
                continue;
            }

            // Priority 1: Check L2LookupJobs
            if (shard.lookup_count > 0) {
                lookup_job = shard.lookup_slots[shard.lookup_head];
                shard.lookup_head = (shard.lookup_head + 1) % kRuntimeQueueSlotsPerShard;
                shard.lookup_count--;
                found_work = true;
                is_lookup = true;
                selected_shard = &shard;

                if (q->resources.runtime_metrics != nullptr) {
                    q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                        1, std::memory_order_relaxed);
                }
                break; // Process lookup job immediately
            }

            // Priority 2: Check L2StoreJobs
            if (shard.store_count > 0) {
                store_job = shard.store_slots[shard.store_head];
                shard.store_head = (shard.store_head + 1) % kRuntimeQueueSlotsPerShard;
                shard.store_count--;
                found_work = true;
                is_lookup = false;
                selected_shard = &shard;

                if (q->resources.runtime_metrics != nullptr) {
                    q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                        1, std::memory_order_relaxed);
                }
                break; // Process store job
            }
        }

        if (found_work) {
            if (is_lookup) {
                execute_lookup_job(q, selected_shard, lookup_job,
                                   q->worker_scratch[worker_id].l2_lookup_body,
                                   kAsyncL2MaxBodySize);
            } else {
                execute_store_job(q, selected_shard, store_job);
            }
            continue;
        }

        // No work found in any owned shard. Wait on the worker's own wake CV.
        WorkerWakeState& wake = q->worker_wakes[worker_id];
        std::unique_lock<std::mutex> lock(wake.mu);
        wake.cv.wait(lock, [q, worker_id] {
            return q->worker_wakes[worker_id].signaled ||
                   !q->running.load(std::memory_order_acquire);
        });
        wake.signaled = false;

        if (!q->running.load(std::memory_order_acquire)) {
            // Check if ALL shards assigned to this worker are empty before exiting.
            bool all_empty = true;
            for (std::size_t i = 0; i < owned_count; ++i) {
                const std::size_t s_idx = q->worker_owned_shards[worker_id][i];
                if (q->shards[s_idx].lookup_count > 0 || q->shards[s_idx].store_count > 0) {
                    all_empty = false;
                    break;
                }
            }
            if (all_empty) {
                return;
            }
        }
    }
}

// Primitives for managing the Ready Shard Queues.
// Lock-ordering convention: never hold RuntimeShard::mu while acquiring WorkerReadyQueue::mu.
[[maybe_unused]] static bool worker_ready_try_push(WorkerQueue* q, std::size_t worker_id,
                                                   std::size_t shard_id) {
    if (q == nullptr || worker_id >= q->worker_count) {
        return false;
    }
    auto& rq = q->worker_ready[worker_id];
    std::scoped_lock lock(rq.mu);
    if (rq.count >= kRuntimeShardCount) {
        return false; // full
    }
    rq.shard_ids[rq.tail] = shard_id;
    rq.tail = (rq.tail + 1) % kRuntimeShardCount;
    rq.count++;
    rq.cv.notify_one();
    return true;
}

[[maybe_unused]] static bool worker_ready_try_pop(WorkerQueue* q, std::size_t worker_id,
                                                  std::size_t* shard_id_out) {
    if (q == nullptr || worker_id >= q->worker_count || shard_id_out == nullptr) {
        return false;
    }
    auto& rq = q->worker_ready[worker_id];
    std::scoped_lock lock(rq.mu);
    if (rq.count == 0) {
        return false;
    }
    *shard_id_out = rq.shard_ids[rq.head];
    rq.head = (rq.head + 1) % kRuntimeShardCount;
    rq.count--;
    return true;
}

[[maybe_unused]] static bool worker_ready_wait_pop(WorkerQueue* q, std::size_t worker_id,
                                                   std::size_t* shard_id_out) {
    if (q == nullptr || worker_id >= q->worker_count || shard_id_out == nullptr) {
        return false;
    }
    auto& rq = q->worker_ready[worker_id];
    std::unique_lock<std::mutex> lock(rq.mu);
    rq.cv.wait(lock,
               [q, &rq] { return rq.count > 0 || !q->running.load(std::memory_order_acquire); });
    if (rq.count == 0) {
        return false; // running was set to false or queue shutdown
    }
    *shard_id_out = rq.shard_ids[rq.head];
    rq.head = (rq.head + 1) % kRuntimeShardCount;
    rq.count--;
    return true;
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
    q->running.store(false, std::memory_order_release);

    // Reset ownership arrays, worker wakes, and worker ready queues
    for (std::size_t w = 0; w < kWorkerQueueMaxWorkers; ++w) {
        q->worker_owned_shard_count[w] = 0;
        q->worker_wakes[w].signaled = false;
        for (std::size_t s = 0; s < kRuntimeMaxShardsPerWorker; ++s) {
            q->worker_owned_shards[w][s] = 0;
        }
        q->worker_ready[w].head = 0;
        q->worker_ready[w].tail = 0;
        q->worker_ready[w].count = 0;
        std::memset(q->worker_ready[w].shard_ids, 0, sizeof(q->worker_ready[w].shard_ids));
    }

    // Populate ownership arrays
    for (std::size_t s_idx = 0; s_idx < kRuntimeShardCount; ++s_idx) {
        std::size_t w_id = s_idx % config.worker_count;
        q->worker_owned_shards[w_id][q->worker_owned_shard_count[w_id]] = s_idx;
        q->worker_owned_shard_count[w_id]++;
    }

    for (std::size_t i = 0; i < kRuntimeShardCount; ++i) {
        q->shards[i].lookup_head = 0;
        q->shards[i].lookup_tail = 0;
        q->shards[i].lookup_count = 0;
        q->shards[i].store_head = 0;
        q->shards[i].store_tail = 0;
        q->shards[i].store_count = 0;
        q->shards[i].pending_count = 0;
        for (std::size_t j = 0; j < kRuntimePendingSlotsPerShard; j++) {
            q->shards[i].pending_occupied[j] = false;
        }
        for (std::size_t j = 0; j < kRuntimeQueueSlotsPerShard; j++) {
            q->shards[i].body_pool.occupied[j] = false;
        }
        q->shards[i].ready_enqueued = false;
    }

    return nullptr;
}

const char* worker_queue_start(WorkerQueue* q, const WorkerQueueResources& res) {
    if (q == nullptr) {
        return "queue pointer is null";
    }

    if (q->running.load(std::memory_order_acquire)) {
        return "queue already running";
    }

    q->running.store(true, std::memory_order_release);
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

bool worker_queue_try_enqueue_lookup(WorkerQueue* q, const L2LookupJob& job) {
    if (q == nullptr || !q->running.load(std::memory_order_acquire)) {
        return false;
    }

    std::uint32_t hash = hash_key(job.key);
    std::uint32_t shard_idx = hash % kRuntimeShardCount;
    RuntimeShard& shard = q->shards[shard_idx];

    {
        std::lock_guard<std::mutex> lock(shard.mu);
        if (!shard_pending_try_mark(&shard, job.key, hash)) {
            return false; // Already pending or registry full
        }

        if (shard.lookup_count >= kRuntimeQueueSlotsPerShard) {
            shard_pending_clear(&shard, job.key, hash);
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

        L2LookupJob mutable_job = job;
        mutable_job.key_hash = hash;
        shard.lookup_slots[shard.lookup_tail] = mutable_job;
        shard.lookup_tail = (shard.lookup_tail + 1) % kRuntimeQueueSlotsPerShard;
        shard.lookup_count++;
        notify_worker_for_shard(q, shard_idx);
    }

    return true;
}

bool worker_queue_try_enqueue_store(WorkerQueue* q, const L2StoreJob& job) {
    if (q == nullptr || !q->running.load(std::memory_order_acquire)) {
        return false;
    }

    std::uint32_t shard_idx = hash_key_to_shard(job.key);
    RuntimeShard& shard = q->shards[shard_idx];

    {
        std::lock_guard<std::mutex> lock(shard.mu);
        if (shard.store_count >= kRuntimeQueueSlotsPerShard) {
            ::bytetaper::metrics::record_runtime_event(
                q->resources.runtime_metrics,
                ::bytetaper::metrics::RuntimeMetricEvent::EnqueueDropped);
            return false;
        }

        // Find available body pool slot
        int free_slot = -1;
        for (std::size_t i = 0; i < kRuntimeQueueSlotsPerShard; ++i) {
            if (!shard.body_pool.occupied[i]) {
                free_slot = static_cast<int>(i);
                break;
            }
        }

        if (free_slot == -1) {
            // Should never occur since store_count < capacity
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

        shard.store_slots[shard.store_tail] = job;
        shard.store_slots[shard.store_tail].body_slot = static_cast<std::uint32_t>(free_slot);

        // Copy body into shard body pool slot
        if (job.entry.body != nullptr && job.body_len > 0) {
            std::size_t copy_len =
                (job.body_len > kAsyncL2MaxBodySize) ? kAsyncL2MaxBodySize : job.body_len;
            std::memcpy(shard.body_pool.bodies[free_slot], job.entry.body, copy_len);
            shard.store_slots[shard.store_tail].body_len = copy_len;
            shard.store_slots[shard.store_tail].entry.body_len = copy_len;
        }
        shard.body_pool.occupied[free_slot] = true;

        // Fix entry pointer to point directly to shard-local body pool
        shard.store_slots[shard.store_tail].entry.body = shard.body_pool.bodies[free_slot];

        shard.store_tail = (shard.store_tail + 1) % kRuntimeQueueSlotsPerShard;
        shard.store_count++;
        notify_worker_for_shard(q, shard_idx);
    }

    return true;
}

void worker_queue_shutdown(WorkerQueue* q) {
    if (q == nullptr) {
        return;
    }

    q->running.store(false, std::memory_order_release);

    for (std::size_t i = 0; i < q->worker_count; ++i) {
        WorkerWakeState& wake = q->worker_wakes[i];
        {
            std::lock_guard<std::mutex> lock(wake.mu);
            wake.signaled = true;
        }
        wake.cv.notify_all();
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

        // Execute lookup job first
        if (shard.lookup_count > 0) {
            L2LookupJob job = shard.lookup_slots[shard.lookup_head];
            shard.lookup_head = (shard.lookup_head + 1) % kRuntimeQueueSlotsPerShard;
            shard.lookup_count--;

            if (q->resources.runtime_metrics != nullptr) {
                q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                    1, std::memory_order_relaxed);
            }

            lock.unlock(); // Release lock before executing I/O
            execute_lookup_job(q, &shard, job, q->worker_scratch[0].l2_lookup_body,
                               kAsyncL2MaxBodySize);
            return true;
        }

        // Execute store job second
        if (shard.store_count > 0) {
            L2StoreJob job = shard.store_slots[shard.store_head];
            shard.store_head = (shard.store_head + 1) % kRuntimeQueueSlotsPerShard;
            shard.store_count--;

            if (q->resources.runtime_metrics != nullptr) {
                q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                    1, std::memory_order_relaxed);
            }

            lock.unlock(); // Release lock before executing I/O
            execute_store_job(q, &shard, job);
            return true;
        }
    }

    return false;
}

} // namespace bytetaper::runtime
