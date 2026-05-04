// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_key.h"
#include "cache/l1_cache.h"
#include "stages/coalescing_follower_wait_stage.h"

#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>

namespace bytetaper::stages {

class CoalescingFollowerWaitTest : public ::testing::Test {
protected:
    void SetUp() override {
        l1_cache = std::make_unique<cache::L1Cache>();
        cache::l1_init(l1_cache.get());

        policy.cache.behavior = policy::CacheBehavior::Store;
        policy.route_id = "12345";

        ctx.matched_policy = &policy;
        ctx.l1_cache = l1_cache.get();
        std::strcpy(ctx.raw_path, "/path");
        ctx.request_method = policy::HttpMethod::Get;

        policy.coalescing.enabled = true;
        policy.coalescing.wait_window_ms = 50;

        auto now = std::chrono::system_clock::now();
        ctx.request_epoch_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        ctx.coalescing_decision.action = coalescing::CoalescingAction::Follower;
        std::strcpy(ctx.coalescing_decision.key, "c_key:test:1:/path");
    }

    std::unique_ptr<cache::L1Cache> l1_cache;
    policy::RoutePolicy policy;
    apg::ApgTransformContext ctx;
};

TEST_F(CoalescingFollowerWaitTest, NonFollowerBypasses) {
    ctx.coalescing_decision.action = coalescing::CoalescingAction::Leader;
    auto output = coalescing_follower_wait_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::Continue);
}

TEST_F(CoalescingFollowerWaitTest, ImmediateHitReturnsSkip) {
    // Populate cache first
    cache::CacheEntry entry;
    std::strcpy(entry.content_type, "application/json");
    entry.body = "{\"data\": \"cached\"}";
    entry.body_len = std::strlen(entry.body);
    entry.created_at_epoch_ms = ctx.request_epoch_ms;
    entry.expires_at_epoch_ms = ctx.request_epoch_ms + 60000;

    cache::CacheKeyInput ki{};
    ki.method = ctx.request_method;
    ki.route_id = policy.route_id;
    ki.path = ctx.raw_path;
    ki.policy_version = policy.route_id;

    cache::build_cache_key(ki, entry.key, sizeof(entry.key));

    cache::l1_put(l1_cache.get(), entry);

    auto output = coalescing_follower_wait_stage(ctx);
    EXPECT_EQ(output.result, apg::StageResult::SkipRemaining);
    EXPECT_TRUE(ctx.cache_hit);
    EXPECT_STREQ(ctx.cache_layer, "L1");
}

TEST_F(CoalescingFollowerWaitTest, TimeoutReturnsContinue) {
    // Ensure cache is empty
    policy.coalescing.wait_window_ms = 20; // Short wait

    auto start = std::chrono::steady_clock::now();
    auto output = coalescing_follower_wait_stage(ctx);
    auto end = std::chrono::steady_clock::now();

    EXPECT_EQ(output.result, apg::StageResult::Continue);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration, 20);
}

} // namespace bytetaper::stages
