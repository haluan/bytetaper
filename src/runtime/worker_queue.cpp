// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "runtime/worker_queue.h"

#include <cstring>

namespace bytetaper::runtime {

namespace {

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
        }

        // Stub: BT-035-005 and BT-035-006 will add L2Lookup/L2Store dispatch here.
        (void) job;
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
    q->dropped_count.store(0, std::memory_order_relaxed);

    return nullptr;
}

const char* worker_queue_start(WorkerQueue* q) {
    if (q == nullptr) {
        return "queue pointer is null";
    }

    std::lock_guard<std::mutex> lock(q->mu);
    if (q->running) {
        return "queue already running";
    }

    q->running = true;
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
        q->dropped_count.fetch_add(1, std::memory_order_relaxed);
        return false;
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

} // namespace bytetaper::runtime
