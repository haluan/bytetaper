// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_matcher.h"
#include "policy/route_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(RouteMatcherTest, PrefixMatchReturnsFirstPolicy) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "p1";
    policies[0].match_kind = RouteMatchKind::Prefix;
    policies[0].match_prefix = "/api/v1/";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/api/v1/users");
    ASSERT_NE(matched, nullptr);
    EXPECT_STREQ(matched->route_id, "p1");
}

TEST(RouteMatcherTest, PrefixMatchWithTrailingSlash) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "p1";
    policies[0].match_kind = RouteMatchKind::Prefix;
    policies[0].match_prefix = "/api/";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/api/");
    ASSERT_NE(matched, nullptr);
    EXPECT_STREQ(matched->route_id, "p1");
}

TEST(RouteMatcherTest, ExactMatchReturnsPolicy) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "e1";
    policies[0].match_kind = RouteMatchKind::Exact;
    policies[0].match_prefix = "/health";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/health");
    ASSERT_NE(matched, nullptr);
    EXPECT_STREQ(matched->route_id, "e1");
}

TEST(RouteMatcherTest, ExactMatchDoesNotMatchSubpath) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "e1";
    policies[0].match_kind = RouteMatchKind::Exact;
    policies[0].match_prefix = "/health";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/health/live");
    EXPECT_EQ(matched, nullptr);
}

TEST(RouteMatcherTest, PrefixMatchDoesNotMatchShorterPath) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "p1";
    policies[0].match_kind = RouteMatchKind::Prefix;
    policies[0].match_prefix = "/api/v1/";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/api");
    EXPECT_EQ(matched, nullptr);
}

TEST(RouteMatcherTest, FirstMatchWins) {
    RoutePolicy policies[2] = {};
    policies[0].route_id = "first";
    policies[0].match_kind = RouteMatchKind::Prefix;
    policies[0].match_prefix = "/api/";

    policies[1].route_id = "second";
    policies[1].match_kind = RouteMatchKind::Prefix;
    policies[1].match_prefix = "/api/v1/";

    // Both match, but the first one in the array should win.
    const RoutePolicy* matched = match_route_by_path(policies, 2, "/api/v1/users");
    ASSERT_NE(matched, nullptr);
    EXPECT_STREQ(matched->route_id, "first");
}

TEST(RouteMatcherTest, NoMatchReturnsNullptr) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "p1";
    policies[0].match_kind = RouteMatchKind::Prefix;
    policies[0].match_prefix = "/api/";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/unknown");
    EXPECT_EQ(matched, nullptr);
}

TEST(RouteMatcherTest, NullPathReturnsNullptr) {
    RoutePolicy policies[1] = {};
    policies[0].match_prefix = "/";
    EXPECT_EQ(match_route_by_path(policies, 1, nullptr), nullptr);
}

TEST(RouteMatcherTest, EmptyPoliciesReturnsNullptr) {
    EXPECT_EQ(match_route_by_path(nullptr, 0, "/api"), nullptr);
}

TEST(RouteMatcherTest, ExactMatchReturnsMutationMode) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "e1";
    policies[0].match_kind = RouteMatchKind::Exact;
    policies[0].match_prefix = "/health";
    policies[0].mutation = MutationMode::HeadersOnly;

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/health");
    ASSERT_NE(matched, nullptr);
    EXPECT_EQ(matched->mutation, MutationMode::HeadersOnly);
}

TEST(RouteMatcherTest, PrefixMatchEmptyPrefix) {
    RoutePolicy policies[1] = {};
    policies[0].route_id = "catch-all";
    policies[0].match_kind = RouteMatchKind::Prefix;
    policies[0].match_prefix = "";

    const RoutePolicy* matched = match_route_by_path(policies, 1, "/anything");
    ASSERT_NE(matched, nullptr);
    EXPECT_STREQ(matched->route_id, "catch-all");
}

} // namespace bytetaper::policy
