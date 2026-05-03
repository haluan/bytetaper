// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_decision.h"
#include "pagination/pagination_mutation.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::pagination {

static policy::PaginationPolicy make_policy(std::uint32_t def, std::uint32_t max) {
    policy::PaginationPolicy p{};
    p.enabled = true;
    p.mode = policy::PaginationMode::LimitOffset;
    p.default_limit = def;
    p.max_limit = max;
    p.upstream_supports_pagination = true;
    return p;
}

TEST(MaxLimitEnforcementTest, LimitExceedsMax_IsCapped) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 5000;
    auto d = make_pagination_decision(make_policy(20, 500), q);
    EXPECT_TRUE(d.should_cap_limit);
    EXPECT_EQ(d.limit_to_apply, 500u);
    EXPECT_STREQ(d.reason, "limit_exceeds_max");

    char buf[256]{};
    auto r =
        apply_pagination_mutation("/orders", 7, "limit=5000", 10, d, "limit", buf, sizeof(buf));
    EXPECT_TRUE(r.mutated);
    EXPECT_STREQ(buf, "/orders?limit=500");
}

TEST(MaxLimitEnforcementTest, LimitEqualsMax_Preserved) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 500;
    auto d = make_pagination_decision(make_policy(20, 500), q);
    EXPECT_FALSE(d.should_cap_limit);
    EXPECT_FALSE(d.should_apply_default_limit);
}

TEST(MaxLimitEnforcementTest, LimitBelowMax_Preserved) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 100;
    auto d = make_pagination_decision(make_policy(20, 500), q);
    EXPECT_FALSE(d.should_cap_limit);
}

TEST(MaxLimitEnforcementTest, MaxLimitZero_NoEnforcement) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 99999;
    auto d = make_pagination_decision(make_policy(20, 0), q);
    EXPECT_FALSE(d.should_cap_limit); // max_limit=0 means no enforcement
}

TEST(MaxLimitEnforcementTest, MutationDeterministic) {
    PaginationDecision d{ false, true, 500, "limit_exceeds_max" };
    char buf1[256]{}, buf2[256]{};
    apply_pagination_mutation("/o", 2, "limit=9999", 10, d, "limit", buf1, sizeof(buf1));
    apply_pagination_mutation("/o", 2, "limit=9999", 10, d, "limit", buf2, sizeof(buf2));
    EXPECT_STREQ(buf1, buf2);
}

} // namespace bytetaper::pagination
