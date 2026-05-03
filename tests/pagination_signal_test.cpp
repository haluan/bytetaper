// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "api_intelligence/pagination_signal.h"

#include <gtest/gtest.h>

namespace bytetaper::api_intelligence {

TEST(PaginationSignalTest, NoSignals_Low) {
    PaginationSignals s{};
    EXPECT_EQ(assess_pagination_risk(s), PaginationRiskLevel::Low);
}

TEST(PaginationSignalTest, OversizedWarning_High) {
    PaginationSignals s{};
    s.oversized_response_warning_seen = true;
    EXPECT_EQ(assess_pagination_risk(s), PaginationRiskLevel::High);
}

TEST(PaginationSignalTest, UnsupportedUpstream_AndMissingLimit_High) {
    PaginationSignals s{};
    s.upstream_pagination_unsupported_seen = true;
    s.missing_limit_seen = true;
    EXPECT_EQ(assess_pagination_risk(s), PaginationRiskLevel::High);
}

TEST(PaginationSignalTest, UnsupportedUpstream_Alone_Low) {
    // Unsupported upstream without missing limit is not HIGH.
    PaginationSignals s{};
    s.upstream_pagination_unsupported_seen = true;
    EXPECT_EQ(assess_pagination_risk(s), PaginationRiskLevel::Low);
}

TEST(PaginationSignalTest, MaxLimitEnforced_Medium) {
    PaginationSignals s{};
    s.max_limit_enforced_seen = true;
    EXPECT_EQ(assess_pagination_risk(s), PaginationRiskLevel::Medium);
}

TEST(PaginationSignalTest, MissingLimitOnly_Low) {
    // missing_limit alone is not elevated risk — default was injected successfully.
    PaginationSignals s{};
    s.missing_limit_seen = true;
    EXPECT_EQ(assess_pagination_risk(s), PaginationRiskLevel::Low);
}

} // namespace bytetaper::api_intelligence
