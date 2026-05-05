// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "runtime/worker_queue.h"

#include "cache/l1_cache.h"
#include "cache/l2_disk_cache.h"
#include "metrics/runtime_metrics.h"
#include "runtime/pending_lookup_registry.h"

#include <chrono>
#include <cstring>

namespace bytetaper::runtime {

namespace {

static void execute_job_internal(WorkerQueue* q, RuntimeCacheJob& job) {
    ::bytetaper::metrics::record_runtime_event(
        q->resources.runtime_metrics, ::bytetaper::metrics::RuntimeMetricEvent::JobExecuted);

    // Dispatch based on job kind
    if (job.kind == RuntimeJobKind::L2Lookup) {
        auto* l1 = q->resources.l1_cache;
        auto* l2 = q->resources.l2_cache;
        auto* pend = q->resources.pending;
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

        if (pend != nullptr) {
            pending_lookup_clear(pend, job.entry.key);
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

static void worker_loop(WorkerQueue* q) {
    while (true) {
        RuntimeCacheJob job;
        {
            std::unique_lock<std::mutex> lock(q->mu);
            q->cv.wait(lock, [q] { return q->count > 0 || !q->running; });

            if (!q->running && q->count == 0) {
                return; // Drain then exit
            }

            job = q->slots[q->head];
            job.entry.body = job.body; // Fix pointer in local copy

            q->head = (q->head + 1) % q->capacity;
            q->count--;

            if (q->resources.runtime_metrics != nullptr) {
                q->resources.runtime_metrics->worker_queue_depth.fetch_sub(
                    1, std::memory_order_relaxed);
            }
        }

        execute_job_internal(q, job);
    }
}

} // namespace

const char* worker_queue_init(WorkerQueue* q, const WorkerQueueConfig& config) {
    if (q == nullptr) {
        return "queue pointer is null";
    }

    if (config.capacity == 0 || config.capacity > kWorkerQueueMaxCapacity) {
        return "invalid capacity";
    }

    if (config.worker_count == 0 || config.worker_count > kWorkerQueueMaxWorkers) {
        return "invalid worker_count";
    }

    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->capacity = config.capacity;
    q->worker_count = config.worker_count;
    q->running = false;

    return nullptr;
}

const char* worker_queue_start(WorkerQueue* q, const WorkerQueueResources& res) {
    if (q == nullptr) {
        return "queue pointer is null";
    }

    std::lock_guard<std::mutex> lock(q->mu);
    if (q->running) {
        return "queue already running";
    }

    q->running = true;
    q->resources = res;
    if (res.runtime_metrics != nullptr) {
        res.runtime_metrics->worker_queue_capacity.store(q->capacity, std::memory_order_relaxed);
    }
    for (std::size_t i = 0; i < q->worker_count; ++i) {
        q->workers[i] = std::thread(worker_loop, q);
    }

    return nullptr;
}

bool worker_queue_try_enqueue(WorkerQueue* q, const RuntimeCacheJob& job) {
    if (q == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(q->mu);
    if (!q->running || q->count >= q->capacity) {
        ::bytetaper::metrics::record_runtime_event(
            q->resources.runtime_metrics, ::bytetaper::metrics::RuntimeMetricEvent::EnqueueDropped);
        return false;
    }

    ::bytetaper::metrics::record_runtime_event(q->resources.runtime_metrics,
                                               ::bytetaper::metrics::RuntimeMetricEvent::Enqueue);
    if (q->resources.runtime_metrics != nullptr) {
        q->resources.runtime_metrics->worker_queue_depth.fetch_add(1, std::memory_order_relaxed);
    }

    q->slots[q->tail] = job;
    // Fix pointer to point to the owned buffer in the slot
    q->slots[q->tail].entry.body = q->slots[q->tail].body;

    // Copy body data if present
    if (job.entry.body != nullptr && job.body_len > 0) {
        std::size_t copy_len =
            (job.body_len > kAsyncL2MaxBodySize) ? kAsyncL2MaxBodySize : job.body_len;
        std::memcpy(q->slots[q->tail].body, job.entry.body, copy_len);
        q->slots[q->tail].body_len = copy_len;
        q->slots[q->tail].entry.body_len = copy_len;
    }

    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    q->cv.notify_one();

    return true;
}

void worker_queue_shutdown(WorkerQueue* q) {
    if (q == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(q->mu);
        if (!q->running) {
            return;
        }
        q->running = false;
    }

    q->cv.notify_all();

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

    RuntimeCacheJob job;
    {
        std::lock_guard<std::mutex> lock(q->mu);
        if (q->count == 0) {
            return false;
        }

        job = q->slots[q->head];
        job.entry.body = job.body;

        q->head = (q->head + 1) % q->capacity;
        q->count--;

        if (q->resources.runtime_metrics != nullptr) {
            q->resources.runtime_metrics->worker_queue_depth.fetch_sub(1,
                                                                       std::memory_order_relaxed);
        }
    }

    execute_job_internal(q, job);
    return true;
}

} // namespace bytetaper::runtime
