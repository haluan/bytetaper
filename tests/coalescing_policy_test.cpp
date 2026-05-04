// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/coalescing_policy.h"
#include "policy/route_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(CoalescingPolicyTest, DefaultValues) {
    CoalescingPolicy p{};
    EXPECT_FALSE(p.enabled);
    EXPECT_EQ(p.mode, CoalescingMode::CacheAssisted);
    EXPECT_EQ(p.wait_window_ms, 25);
    EXPECT_EQ(p.max_waiters_per_key, 64);
    EXPECT_TRUE(p.require_cache_enabled);
    EXPECT_FALSE(p.allow_authenticated);
}

TEST(CoalescingPolicyTest, Validation_DisabledIsAlwaysValid) {
    CoalescingPolicy p{};
    p.enabled = false;
    p.wait_window_ms = 0; // invalid if enabled
    EXPECT_EQ(validate_coalescing_policy(p), nullptr);
}

TEST(CoalescingPolicyTest, Validation_ValidEnabled) {
    CoalescingPolicy p{};
    p.enabled = true;
    p.wait_window_ms = 100;
    p.max_waiters_per_key = 10;
    p.require_cache_enabled = false;
    EXPECT_EQ(validate_coalescing_policy_safe(p), nullptr);
}

TEST(CoalescingPolicyTest, Validation_InvalidWaitWindow) {
    CoalescingPolicy p{};
    p.enabled = true;
    p.wait_window_ms = 0;
    EXPECT_STREQ(validate_coalescing_policy(p), "coalescing.wait_window_ms must be > 0");

    p.wait_window_ms = 101;
    EXPECT_STREQ(validate_coalescing_policy_safe(p),
                 "coalescing.wait_window_ms exceeds safe production limit (100ms)");

    p.wait_window_ms = 6000;
    EXPECT_STREQ(validate_coalescing_policy(p),
                 "coalescing.wait_window_ms exceeds maximum allowed wait (5000ms)");
}

TEST(CoalescingPolicyTest, Validation_InvalidMaxWaiters) {
    CoalescingPolicy p{};
    p.enabled = true;
    p.max_waiters_per_key = 0;
    EXPECT_STREQ(validate_coalescing_policy(p), "coalescing.max_waiters_per_key must be > 0");
}

TEST(RoutePolicyIntegrationTest, CoalescingRequiresCache) {
    RoutePolicy rp{};
    rp.route_id = "test";
    rp.match_prefix = "/";
    rp.cache.enabled = false;
    rp.coalescing.enabled = true;
    rp.coalescing.require_cache_enabled = true;

    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(rp, &reason));
    EXPECT_STREQ(reason, "coalescing requires cache to be enabled");

    rp.cache.enabled = true;
    EXPECT_TRUE(validate_route_policy(rp, &reason));
}

TEST(RoutePolicyIntegrationTest, CoalescingNoCacheRequired) {
    RoutePolicy rp{};
    rp.route_id = "test";
    rp.match_prefix = "/";
    rp.cache.enabled = false;
    rp.coalescing.enabled = true;
    rp.coalescing.require_cache_enabled = false;

    const char* reason = nullptr;
    EXPECT_TRUE(validate_route_policy(rp, &reason));
}

} // namespace bytetaper::policy
