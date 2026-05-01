// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/cache_policy.h"
#include "policy/route_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(CachePolicyTest, DefaultBehaviorIsDefault) {
    CachePolicy policy{};
    EXPECT_EQ(policy.behavior, CacheBehavior::Default);
}

TEST(CachePolicyTest, DefaultTtlIsZero) {
    CachePolicy policy{};
    EXPECT_EQ(policy.ttl_seconds, 0u);
}

TEST(CachePolicyTest, DefaultRoutePolicyCacheIsDefault) {
    RoutePolicy policy{};
    EXPECT_EQ(policy.cache.behavior, CacheBehavior::Default);
    EXPECT_EQ(policy.cache.ttl_seconds, 0u);
}

} // namespace bytetaper::policy
