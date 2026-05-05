// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/pipeline.h"
#include "cache/l1_cache.h"
#include "coalescing/inflight_registry.h"
#include "policy/route_policy.h"
#include "stages/coalescing_follower_wait_stage.h"

#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>

namespace bytetaper::stages {

TEST(CoalescingEventDrivenWait, FollowerWakesBeforeFullWindowWhenLeaderCompletes) {
    auto registry = std::make_unique<coalescing::InFlightRegistry>();
    coalescing::registry_init(registry.get());

    auto l1 = std::make_unique<cache::L1Cache>();
    cache::l1_init(l1.get());

    policy::RoutePolicy policy{};
    policy.route_id = "test-route";
    policy.cache.behavior = policy::CacheBehavior::Store;
    policy.coalescing.enabled = true;
    policy.coalescing.wait_window_ms = 1000; // 1 second window

    apg::ApgTransformContext ctx{};
    ctx.matched_policy = &policy;
    ctx.l1_cache = l1.get();
    ctx.coalescing_registry = registry.get();
    ctx.coalescing_decision.action = coalescing::CoalescingAction::Follower;
    std::strcpy(ctx.coalescing_decision.key, "burst-key");

    // Pre-register leader so follower stage enters wait
    coalescing::registry_register(registry.get(), "burst-key", 1000, 1000, 128);

    auto start = std::chrono::steady_clock::now();

    // Start a thread to complete the leader after 50ms
    std::thread leader_thread([&registry]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        coalescing::registry_complete(registry.get(), "burst-key", true, 1050);
    });

    // This should block and wake up when the leader completes
    apg::StageOutput output = coalescing_follower_wait_stage(ctx);

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    leader_thread.join();

    // Should wake up much faster than 1 second (e.g. < 500ms)
    EXPECT_LT(elapsed_ms, 500);
    // In this test, it will fallback because we didn't actually put anything in L1
    EXPECT_STREQ(output.note, "leader-complete-l1-miss");
}

TEST(CoalescingEventDrivenWait, FollowerTimeoutFallbackStillWorks) {
    auto registry = std::make_unique<coalescing::InFlightRegistry>();
    coalescing::registry_init(registry.get());

    auto l1 = std::make_unique<cache::L1Cache>();
    cache::l1_init(l1.get());

    policy::RoutePolicy policy{};
    policy.route_id = "test-route";
    policy.cache.behavior = policy::CacheBehavior::Store;
    policy.coalescing.enabled = true;
    policy.coalescing.wait_window_ms = 100; // 100ms window

    apg::ApgTransformContext ctx{};
    ctx.matched_policy = &policy;
    ctx.l1_cache = l1.get();
    ctx.coalescing_registry = registry.get();
    ctx.coalescing_decision.action = coalescing::CoalescingAction::Follower;
    std::strcpy(ctx.coalescing_decision.key, "timeout-key");

    // Register leader, but never complete it
    coalescing::registry_register(registry.get(), "timeout-key", 1000, 100, 128);

    auto start = std::chrono::steady_clock::now();

    apg::StageOutput output = coalescing_follower_wait_stage(ctx);

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Should wait at least 100ms
    EXPECT_GE(elapsed_ms, 100);
    EXPECT_STREQ(output.note, "timeout-fallback");
}

} // namespace bytetaper::stages
