// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/compression_size.h"

#include <gtest/gtest.h>

namespace bytetaper::compression {

TEST(CompressionSizeTest, BelowThreshold_NotEligible) {
    auto r = check_compression_size_eligibility(1024, 1023, true);
    EXPECT_FALSE(r.eligible);
    EXPECT_STREQ(r.reason, "below_minimum");
}

TEST(CompressionSizeTest, EqualThreshold_Eligible) {
    auto r = check_compression_size_eligibility(1024, 1024, true);
    EXPECT_TRUE(r.eligible);
    EXPECT_EQ(r.reason, nullptr);
}

TEST(CompressionSizeTest, AboveThreshold_Eligible) {
    auto r = check_compression_size_eligibility(1024, 4096, true);
    EXPECT_TRUE(r.eligible);
    EXPECT_EQ(r.reason, nullptr);
}

TEST(CompressionSizeTest, UnknownSize_NotEligible) {
    auto r = check_compression_size_eligibility(1024, 0, false);
    EXPECT_FALSE(r.eligible);
    EXPECT_STREQ(r.reason, "size_unknown");
}

TEST(CompressionSizeTest, ZeroMinSize_AlwaysEligible) {
    // min_size_bytes=0 disables size check.
    auto r = check_compression_size_eligibility(0, 1, true);
    EXPECT_TRUE(r.eligible);
    EXPECT_EQ(r.reason, nullptr);
}

TEST(CompressionSizeTest, ReasonDeterministic) {
    // Same inputs always produce the same static reason string.
    auto r1 = check_compression_size_eligibility(1024, 100, true);
    auto r2 = check_compression_size_eligibility(1024, 100, true);
    EXPECT_STREQ(r1.reason, r2.reason);
}

} // namespace bytetaper::compression
