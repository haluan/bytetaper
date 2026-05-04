// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(RoutePolicyTest, ValidPolicyDefaultMutation) {
    RoutePolicy policy{};
    policy.route_id = "test-route";
    policy.match_prefix = "/api";

    const char* reason = nullptr;
    EXPECT_TRUE(validate_route_policy(policy, &reason));
}

TEST(RoutePolicyTest, ValidPolicyHeadersOnlyMutation) {
    RoutePolicy policy{};
    policy.route_id = "test-route";
    policy.match_prefix = "/api";
    policy.mutation = MutationMode::HeadersOnly;

    EXPECT_TRUE(validate_route_policy(policy, nullptr));
}

TEST(RoutePolicyTest, ValidPolicyFullMutation) {
    RoutePolicy policy{};
    policy.route_id = "test-route";
    policy.match_prefix = "/api";
    policy.mutation = MutationMode::Full;

    EXPECT_TRUE(validate_route_policy(policy, nullptr));
}

TEST(RoutePolicyTest, InvalidPolicyNullRouteId) {
    RoutePolicy policy{};
    policy.route_id = nullptr;

    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(policy, &reason));
    EXPECT_STREQ(reason, "route_id is required");
}

TEST(RoutePolicyTest, InvalidPolicyEmptyRouteId) {
    RoutePolicy policy{};
    policy.route_id = "";

    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(policy, &reason));
    EXPECT_STREQ(reason, "route_id is required");
}

TEST(RoutePolicyTest, InvalidPolicyNullMatchPrefix) {
    RoutePolicy policy{};
    policy.route_id = "ok";
    policy.match_kind = RouteMatchKind::Prefix;
    policy.match_prefix = nullptr;

    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(policy, &reason));
    EXPECT_STREQ(reason, "match_prefix must start with '/'");
}

TEST(RoutePolicyTest, InvalidPolicyBadMatchPrefix) {
    RoutePolicy policy{};
    policy.route_id = "ok";
    policy.match_kind = RouteMatchKind::Prefix;
    policy.match_prefix = "api/v1";

    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(policy, &reason));
    EXPECT_STREQ(reason, "match_prefix must start with '/'");
}

TEST(RoutePolicyTest, DefaultMutationIsDisabled) {
    RoutePolicy policy{};
    EXPECT_EQ(policy.mutation, MutationMode::Disabled);
}

TEST(RoutePolicyTest, InvalidPolicyCoalescingRequiresCache) {
    RoutePolicy policy{};
    policy.route_id = "ok";
    policy.match_prefix = "/";
    policy.coalescing.enabled = true;
    policy.coalescing.require_cache_enabled = true;
    policy.cache.enabled = false;

    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(policy, &reason));
    EXPECT_STREQ(reason, "coalescing requires cache to be enabled");
}

} // namespace bytetaper::policy
