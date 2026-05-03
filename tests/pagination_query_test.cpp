// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_query.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::pagination {

static PaginationQueryResult parse(const char* qs) {
    return parse_pagination_query(qs, std::strlen(qs), "limit", "offset");
}

TEST(PaginationQueryTest, ValidLimitOffset_Parsed) {
    auto r = parse("limit=100&offset=200");
    EXPECT_FALSE(r.parse_error);
    EXPECT_TRUE(r.has_limit);
    EXPECT_EQ(r.limit, 100u);
    EXPECT_TRUE(r.has_offset);
    EXPECT_EQ(r.offset, 200u);
}

TEST(PaginationQueryTest, MissingLimit_Safe) {
    auto r = parse("offset=20&fields=id");
    EXPECT_FALSE(r.parse_error);
    EXPECT_FALSE(r.has_limit);
    EXPECT_TRUE(r.has_offset);
    EXPECT_EQ(r.offset, 20u);
}

TEST(PaginationQueryTest, NonNumericLimit_Error) {
    auto r = parse("limit=abc&offset=10");
    EXPECT_TRUE(r.parse_error);
    EXPECT_STREQ(r.error_param, "limit");
}

TEST(PaginationQueryTest, NegativeLimit_Error) {
    auto r = parse("limit=-1&offset=0");
    EXPECT_TRUE(r.parse_error);
    EXPECT_STREQ(r.error_param, "limit");
}

TEST(PaginationQueryTest, ZeroLimit_Valid) {
    auto r = parse("limit=0&offset=0");
    EXPECT_FALSE(r.parse_error);
    EXPECT_TRUE(r.has_limit);
    EXPECT_EQ(r.limit, 0u);
    EXPECT_TRUE(r.has_offset);
    EXPECT_EQ(r.offset, 0u);
}

TEST(PaginationQueryTest, EmptyQuery_Safe) {
    auto r = parse_pagination_query(nullptr, 0, "limit", "offset");
    EXPECT_FALSE(r.parse_error);
    EXPECT_FALSE(r.has_limit);
    EXPECT_FALSE(r.has_offset);
}

} // namespace bytetaper::pagination
