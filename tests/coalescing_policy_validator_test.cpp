// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/cache_policy.h"
#include "policy/coalescing_policy_validator.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::policy {

class CoalescingPolicyValidatorTest : public ::testing::Test {
protected:
    CoalescingPolicy policy{};
    CachePolicy cache_policy{};

    void SetUp() override {
        policy.enabled = true;
        policy.wait_window_ms = 25;
        policy.max_waiters_per_key = 64;
        policy.require_cache_enabled = false;
        policy.allow_authenticated = false;

        cache_policy.enabled = false;
    }
};

TEST_F(CoalescingPolicyValidatorTest, ValidMinimal) {
    EXPECT_EQ(validate_coalescing_policy_safe(policy, &cache_policy), nullptr);
}

TEST_F(CoalescingPolicyValidatorTest, DisabledPolicyIsAlwaysValid) {
    policy.enabled = false;
    policy.wait_window_ms = 0; // structurally invalid but policy is disabled
    EXPECT_EQ(validate_coalescing_policy_safe(policy, &cache_policy), nullptr);
}

TEST_F(CoalescingPolicyValidatorTest, RejectInvalidWaitWindow) {
    policy.wait_window_ms = 0;
    EXPECT_STREQ(validate_coalescing_policy_safe(policy, &cache_policy),
                 "coalescing.wait_window_ms must be > 0");

    policy.wait_window_ms = 101;
    EXPECT_STREQ(validate_coalescing_policy_safe(policy, &cache_policy),
                 "coalescing.wait_window_ms exceeds safe production limit (100ms)");
}

TEST_F(CoalescingPolicyValidatorTest, RejectInvalidMaxWaiters) {
    policy.max_waiters_per_key = 0;
    EXPECT_STREQ(validate_coalescing_policy_safe(policy, &cache_policy),
                 "coalescing.max_waiters_per_key must be > 0");
}

TEST_F(CoalescingPolicyValidatorTest, RequireCacheDependency) {
    policy.require_cache_enabled = true;
    cache_policy.enabled = false;
    EXPECT_STREQ(validate_coalescing_policy_safe(policy, &cache_policy),
                 "coalescing requires cache to be enabled");

    cache_policy.enabled = true;
    EXPECT_EQ(validate_coalescing_policy_safe(policy, &cache_policy), nullptr);
}

TEST_F(CoalescingPolicyValidatorTest, RequireAuthScopeForAuthenticated) {
    policy.allow_authenticated = true;
    cache_policy.auth_scope_header[0] = '\0';
    EXPECT_STREQ(validate_coalescing_policy_safe(policy, &cache_policy),
                 "coalescing.allow_authenticated=true requires cache.auth_scope_header to be set");

    std::strcpy(cache_policy.auth_scope_header, "x-user-id");
    EXPECT_EQ(validate_coalescing_policy_safe(policy, &cache_policy), nullptr);
}

TEST_F(CoalescingPolicyValidatorTest, RejectNullCachePolicyIfRequired) {
    policy.require_cache_enabled = true;
    EXPECT_STREQ(validate_coalescing_policy_safe(policy, nullptr),
                 "coalescing requires cache to be enabled");
}

} // namespace bytetaper::policy
