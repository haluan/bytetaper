// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "api_intelligence/coalescing_signal.h"

#include <gtest/gtest.h>

namespace bytetaper::api_intelligence {

TEST(CoalescingSignalTest, NoSignals_NoOpportunity) {
    CoalescingSignals s{};
    auto opp = assess_coalescing_opportunity(s);
    EXPECT_FALSE(opp.has_opportunity);
    EXPECT_EQ(opp.recommendation_code, nullptr);
    EXPECT_EQ(opp.recommendation_reason, nullptr);
}

TEST(CoalescingSignalTest, DuplicateGet_RecordsOpportunity) {
    CoalescingSignals s{};
    s.duplicate_get_seen = true;
    auto opp = assess_coalescing_opportunity(s);
    EXPECT_TRUE(opp.has_opportunity);
    ASSERT_NE(opp.recommendation_code, nullptr);
    ASSERT_NE(opp.recommendation_reason, nullptr);
    EXPECT_STREQ(opp.recommendation_code, "enable_request_coalescing");
    EXPECT_STREQ(opp.recommendation_reason, "duplicate_get_burst_seen");
}

TEST(CoalescingSignalTest, FollowerJoined_CoalescingActive_NoNewOpportunity) {
    CoalescingSignals s{};
    s.duplicate_get_seen = true;
    s.follower_joined_seen = true;
    auto opp = assess_coalescing_opportunity(s);
    // Even if duplicates are seen, if a follower joined, coalescing is already working.
    EXPECT_FALSE(opp.has_opportunity);
    EXPECT_EQ(opp.recommendation_code, nullptr);
}

TEST(CoalescingSignalTest, HighFallbackOnly_NoRecommendation) {
    CoalescingSignals s{};
    s.high_fallback_rate_seen = true;
    auto opp = assess_coalescing_opportunity(s);
    EXPECT_FALSE(opp.has_opportunity);
}

TEST(CoalescingSignalTest, TooManyWaitersOnly_NoRecommendation) {
    CoalescingSignals s{};
    s.too_many_waiters_seen = true;
    auto opp = assess_coalescing_opportunity(s);
    EXPECT_FALSE(opp.has_opportunity);
}

} // namespace bytetaper::api_intelligence
