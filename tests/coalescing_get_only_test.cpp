// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_eligibility.h"

#include <gtest/gtest.h>

namespace bytetaper::coalescing {

TEST(CoalescingGetOnlyTest, GetIsEligible) {
    auto result = evaluate_coalescing_eligibility(policy::HttpMethod::Get);
    EXPECT_TRUE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::None);
    EXPECT_EQ(get_rejection_reason_string(result.rejection_reason), "none");
}

TEST(CoalescingGetOnlyTest, PostIsIneligible) {
    auto result = evaluate_coalescing_eligibility(policy::HttpMethod::Post);
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::MethodNotGet);
    EXPECT_EQ(get_rejection_reason_string(result.rejection_reason), "method_not_get");
}

TEST(CoalescingGetOnlyTest, PutIsIneligible) {
    auto result = evaluate_coalescing_eligibility(policy::HttpMethod::Put);
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::MethodNotGet);
}

TEST(CoalescingGetOnlyTest, DeleteIsIneligible) {
    auto result = evaluate_coalescing_eligibility(policy::HttpMethod::Delete);
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::MethodNotGet);
}

TEST(CoalescingGetOnlyTest, PatchIsIneligible) {
    auto result = evaluate_coalescing_eligibility(policy::HttpMethod::Patch);
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::MethodNotGet);
}

TEST(CoalescingGetOnlyTest, UnknownReasonString) {
    EXPECT_EQ(get_rejection_reason_string(static_cast<CoalescingRejectionReason>(99)), "unknown");
}

} // namespace bytetaper::coalescing
