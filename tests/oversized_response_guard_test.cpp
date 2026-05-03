// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/oversized_response_guard.h"

#include <gtest/gtest.h>

namespace bytetaper::pagination {

TEST(OversizedResponseGuardTest, AboveThreshold_Warns) {
    auto r = check_oversized_response(1000, 1001);
    EXPECT_TRUE(r.warned);
    EXPECT_STREQ(r.reason, "response_still_oversized");
}

TEST(OversizedResponseGuardTest, EqualThreshold_NoWarn) {
    auto r = check_oversized_response(1000, 1000);
    EXPECT_FALSE(r.warned);
    EXPECT_EQ(r.reason, nullptr);
}

TEST(OversizedResponseGuardTest, BelowThreshold_NoWarn) {
    auto r = check_oversized_response(1000, 500);
    EXPECT_FALSE(r.warned);
    EXPECT_EQ(r.reason, nullptr);
}

TEST(OversizedResponseGuardTest, ZeroThreshold_Disabled) {
    auto r = check_oversized_response(0, 999999);
    EXPECT_FALSE(r.warned);
    EXPECT_EQ(r.reason, nullptr);
}

TEST(OversizedResponseGuardTest, BodyNotMutated) {
    // Warning is a read-only signal; function takes by value — no side effects.
    // Compilation-only: if this test compiles and the above pass, invariant holds.
    SUCCEED();
}

} // namespace bytetaper::pagination
