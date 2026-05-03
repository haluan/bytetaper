// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_decision.h"

#include <gtest/gtest.h>

namespace bytetaper::pagination {

static policy::PaginationPolicy make_enabled_policy(std::uint32_t default_limit = 20) {
    policy::PaginationPolicy p{};
    p.enabled = true;
    p.mode = policy::PaginationMode::LimitOffset;
    p.default_limit = default_limit;
    p.upstream_supports_pagination = true;
    return p;
}

TEST(MissingLimitDecisionTest, MissingLimit_DefaultApplied) {
    PaginationQueryResult q{};
    q.has_limit = false;
    auto d = make_pagination_decision(make_enabled_policy(20), q);
    EXPECT_TRUE(d.should_apply_default_limit);
    EXPECT_EQ(d.limit_to_apply, 20u);
}

TEST(MissingLimitDecisionTest, ExistingLimit_NoDefault) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 50;
    auto d = make_pagination_decision(make_enabled_policy(20), q);
    EXPECT_FALSE(d.should_apply_default_limit);
}

TEST(MissingLimitDecisionTest, DisabledPolicy_NoMutation) {
    policy::PaginationPolicy p = make_enabled_policy(20);
    p.enabled = false;
    PaginationQueryResult q{};
    auto d = make_pagination_decision(p, q);
    EXPECT_FALSE(d.should_apply_default_limit);
}

TEST(MissingLimitDecisionTest, UnsupportedUpstream_NoMutation) {
    policy::PaginationPolicy p = make_enabled_policy(20);
    p.upstream_supports_pagination = false;
    PaginationQueryResult q{};
    auto d = make_pagination_decision(p, q);
    EXPECT_FALSE(d.should_apply_default_limit);
}

TEST(MissingLimitDecisionTest, ParseError_NoMutation) {
    PaginationQueryResult q{};
    q.parse_error = true;
    auto d = make_pagination_decision(make_enabled_policy(20), q);
    EXPECT_FALSE(d.should_apply_default_limit);
}

} // namespace bytetaper::pagination
