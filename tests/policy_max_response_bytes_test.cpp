// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_policy.h"

#include <cstdint>
#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(MaxResponseBytesTest, ZeroLimitNeverExceeds) {
    RoutePolicy policy{};
    policy.max_response_bytes = 0;
    EXPECT_FALSE(exceeds_max_response_bytes(policy, 1000000));
}

TEST(MaxResponseBytesTest, ZeroLimitWithZeroSize) {
    RoutePolicy policy{};
    policy.max_response_bytes = 0;
    EXPECT_FALSE(exceeds_max_response_bytes(policy, 0));
}

TEST(MaxResponseBytesTest, DefaultPolicyIsUnlimited) {
    RoutePolicy policy{};
    EXPECT_EQ(policy.max_response_bytes, 0u);
}

TEST(MaxResponseBytesTest, SizeEqualToLimitDoesNotExceed) {
    RoutePolicy policy{};
    policy.max_response_bytes = 1000;
    EXPECT_FALSE(exceeds_max_response_bytes(policy, 1000));
}

TEST(MaxResponseBytesTest, SizeOneBelowLimitDoesNotExceed) {
    RoutePolicy policy{};
    policy.max_response_bytes = 1000;
    EXPECT_FALSE(exceeds_max_response_bytes(policy, 999));
}

TEST(MaxResponseBytesTest, SizeOneAboveLimitExceeds) {
    RoutePolicy policy{};
    policy.max_response_bytes = 1000;
    EXPECT_TRUE(exceeds_max_response_bytes(policy, 1001));
}

TEST(MaxResponseBytesTest, LargeLimitDoesNotExceed) {
    RoutePolicy policy{};
    policy.max_response_bytes = 0xFFFFFFFFu;
    EXPECT_FALSE(exceeds_max_response_bytes(policy, 0xFFFFFFFFu));
}

} // namespace bytetaper::policy
