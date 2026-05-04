// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_safety.h"

#include <gtest/gtest.h>

namespace bytetaper::coalescing {

TEST(CoalescingAuthSafetyTest, NoAuthHeadersIsEligible) {
    CoalescingSafetyInput input;
    input.has_authorization_header = false;
    input.has_cookie_header = false;
    input.allow_authenticated = false;

    auto result = evaluate_coalescing_safety(input);
    EXPECT_TRUE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::None);
}

TEST(CoalescingAuthSafetyTest, AuthorizationHeaderRejectsByDefault) {
    CoalescingSafetyInput input;
    input.has_authorization_header = true;
    input.has_cookie_header = false;
    input.allow_authenticated = false;

    auto result = evaluate_coalescing_safety(input);
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::AuthenticatedRequest);
    EXPECT_EQ(get_rejection_reason_string(result.rejection_reason), "authenticated_request");
}

TEST(CoalescingAuthSafetyTest, CookieHeaderRejectsByDefault) {
    CoalescingSafetyInput input;
    input.has_authorization_header = false;
    input.has_cookie_header = true;
    input.allow_authenticated = false;

    auto result = evaluate_coalescing_safety(input);
    EXPECT_FALSE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::AuthenticatedRequest);
}

TEST(CoalescingAuthSafetyTest, AllowAuthenticatedOverridesSafety) {
    CoalescingSafetyInput input;
    input.has_authorization_header = true;
    input.has_cookie_header = true;
    input.allow_authenticated = true;

    auto result = evaluate_coalescing_safety(input);
    EXPECT_TRUE(result.is_eligible);
    EXPECT_EQ(result.rejection_reason, CoalescingRejectionReason::None);
}

} // namespace bytetaper::coalescing
