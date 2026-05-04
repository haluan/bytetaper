// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "api_intelligence/compression_signal.h"

#include <gtest/gtest.h>

namespace bytetaper::api_intelligence {

TEST(CompressionSignalTest, NoSignals_NoOpportunity) {
    CompressionSignals s{};
    auto opp = assess_compression_opportunity(s);
    EXPECT_FALSE(opp.has_opportunity);
    EXPECT_EQ(opp.recommendation_reason, nullptr);
}

TEST(CompressionSignalTest, LargeUncompressed_Eligible_RecordsOpportunity) {
    CompressionSignals s{};
    s.large_uncompressed_response_seen = true;
    s.client_supports_compression_seen = true;
    s.compression_candidate_seen = true;
    auto opp = assess_compression_opportunity(s);
    EXPECT_TRUE(opp.has_opportunity);
    ASSERT_NE(opp.recommendation_reason, nullptr);
    EXPECT_STREQ(opp.recommendation_reason, "large_uncompressed_json_response_seen");
}

TEST(CompressionSignalTest, AlreadyEncoded_DoesNotRecordOpportunity) {
    CompressionSignals s{};
    s.large_uncompressed_response_seen = true;
    s.already_encoded_seen = true;
    auto opp = assess_compression_opportunity(s);
    EXPECT_FALSE(opp.has_opportunity);
    EXPECT_EQ(opp.recommendation_reason, nullptr);
}

TEST(CompressionSignalTest, TooSmall_RecordsSkipReason) {
    CompressionSignals s{};
    s.large_uncompressed_response_seen =
        true; // wait, if it's large but skipped small? let's follow the logic
    s.compression_skipped_too_small_seen = true;
    auto opp = assess_compression_opportunity(s);
    EXPECT_FALSE(opp.has_opportunity);
    ASSERT_NE(opp.recommendation_reason, nullptr);
    EXPECT_STREQ(opp.recommendation_reason, "response_too_small");
}

} // namespace bytetaper::api_intelligence
