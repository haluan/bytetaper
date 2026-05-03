// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_decision.h"
#include "pagination/pagination_mutation.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::pagination {

static policy::PaginationPolicy make_policy(std::uint32_t def = 20, std::uint32_t max = 500) {
    policy::PaginationPolicy p{};
    p.enabled = true;
    p.mode = policy::PaginationMode::LimitOffset;
    p.default_limit = def;
    p.max_limit = max;
    p.upstream_supports_pagination = true;
    return p;
}

TEST(PreserveValidLimitTest, ValidLimitWithinMax_NoMutation) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 100;
    auto d = make_pagination_decision(make_policy(20, 500), q);
    EXPECT_FALSE(d.should_apply_default_limit);
    EXPECT_FALSE(d.should_cap_limit);
}

TEST(PreserveValidLimitTest, ValidLimitAtMax_NoMutation) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 500;
    auto d = make_pagination_decision(make_policy(20, 500), q);
    EXPECT_FALSE(d.should_apply_default_limit);
    EXPECT_FALSE(d.should_cap_limit);
}

TEST(PreserveValidLimitTest, ValidLimitReason_Recorded) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 100;
    auto d = make_pagination_decision(make_policy(), q);
    EXPECT_STREQ(d.reason, "limit_valid");
}

TEST(PreserveValidLimitTest, QueryUnchanged_NoMutationApplied) {
    PaginationQueryResult q{};
    q.has_limit = true;
    q.limit = 100;
    q.has_offset = true;
    q.offset = 200;
    auto d = make_pagination_decision(make_policy(), q);

    char buf[256]{};
    auto r = apply_pagination_mutation("/orders", 7, "limit=100&offset=200", 20, d, "limit", buf,
                                       sizeof(buf));
    EXPECT_FALSE(r.mutated);
    EXPECT_STREQ(buf, "/orders?limit=100&offset=200");
}

} // namespace bytetaper::pagination
