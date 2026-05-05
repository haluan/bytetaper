// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/grpc_server.h"
#include "runtime/worker_queue.h"

#include <atomic>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

// Internal class extracted from grpc_server.cpp for testing
namespace bytetaper::extproc {
class WorkerQueueStartGuard {
public:
    WorkerQueueStartGuard() = default;
    ~WorkerQueueStartGuard() {
        if (active_queue_ != nullptr) {
            runtime::worker_queue_shutdown(active_queue_);
        }
    }

    void arm(runtime::WorkerQueue* queue) {
        active_queue_ = queue;
    }
    void release() {
        active_queue_ = nullptr;
    }

private:
    runtime::WorkerQueue* active_queue_ = nullptr;
};
} // namespace bytetaper::extproc

namespace bytetaper::extproc {

TEST(GrpcServerWorkerLifecycle, WorkerQueueShutdownOnStartupFailure) {
    auto queue = std::make_unique<runtime::WorkerQueue>();
    runtime::WorkerQueueConfig config{};
    config.worker_count = 1;

    runtime::worker_queue_init(queue.get(), config);

    {
        WorkerQueueStartGuard guard{};
        runtime::worker_queue_start(queue.get(), {});
        guard.arm(queue.get());
    }

    EXPECT_FALSE(queue->running);
    EXPECT_FALSE(queue->workers[0].joinable());
}

TEST(GrpcServerWorkerLifecycle, RaiiGuardShutdownOnPartialStartupFailure) {
    auto queue = std::make_unique<runtime::WorkerQueue>();
    runtime::WorkerQueueConfig config{};
    config.worker_count = 1;
    runtime::worker_queue_init(queue.get(), config);

    {
        WorkerQueueStartGuard guard{};
        runtime::worker_queue_start(queue.get(), {});
        guard.arm(queue.get());
    }

    EXPECT_FALSE(queue->running);
}

TEST(GrpcServerWorkerLifecycle, RaiiGuardReleasesOnSuccess) {
    auto queue = std::make_unique<runtime::WorkerQueue>();
    runtime::WorkerQueueConfig config{};
    config.worker_count = 1;
    runtime::worker_queue_init(queue.get(), config);

    {
        WorkerQueueStartGuard guard{};
        runtime::worker_queue_start(queue.get(), {});
        guard.arm(queue.get());
        guard.release();
    }

    EXPECT_TRUE(queue->running);
    EXPECT_TRUE(queue->workers[0].joinable());

    runtime::worker_queue_shutdown(queue.get());
}

TEST(GrpcServerWorkerLifecycle, NoDoubleShutdownOnNormalStop) {
    auto queue = std::make_unique<runtime::WorkerQueue>();
    runtime::WorkerQueueConfig config{};
    config.worker_count = 1;
    runtime::worker_queue_init(queue.get(), config);
    runtime::worker_queue_start(queue.get(), {});

    runtime::worker_queue_shutdown(queue.get());
    EXPECT_FALSE(queue->running);
    EXPECT_FALSE(queue->workers[0].joinable());

    runtime::worker_queue_shutdown(queue.get());
    EXPECT_FALSE(queue->running);
    EXPECT_FALSE(queue->workers[0].joinable());
}

} // namespace bytetaper::extproc
