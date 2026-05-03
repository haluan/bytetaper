// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_query.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::pagination {

using Mode = policy::PaginationMode;

static PaginationQueryResult parse(const char* qs, const char* offset_param = "offset") {
    return parse_pagination_query(qs, std::strlen(qs), "limit", offset_param, Mode::LimitOffset);
}

TEST(OffsetQueryTest, ValidOffset_Parsed) {
    auto r = parse("limit=50&offset=100");
    EXPECT_FALSE(r.parse_error);
    EXPECT_TRUE(r.has_offset);
    EXPECT_EQ(r.offset, 100u);
}

TEST(OffsetQueryTest, MissingOffset_Allowed) {
    auto r = parse("limit=50");
    EXPECT_FALSE(r.parse_error);
    EXPECT_FALSE(r.has_offset);
}

TEST(OffsetQueryTest, NegativeOffset_Rejected) {
    auto r = parse("limit=50&offset=-1");
    EXPECT_TRUE(r.parse_error);
    EXPECT_STREQ(r.error_param, "offset");
}

TEST(OffsetQueryTest, NonNumericOffset_Rejected) {
    auto r = parse("limit=50&offset=abc");
    EXPECT_TRUE(r.parse_error);
    EXPECT_STREQ(r.error_param, "offset");
}

TEST(OffsetQueryTest, ConfiguredOffsetParam_Respected) {
    // Route uses "skip" instead of "offset"
    auto r = parse_pagination_query("limit=50&skip=200", 17, "limit", "skip", Mode::LimitOffset);
    EXPECT_FALSE(r.parse_error);
    EXPECT_TRUE(r.has_offset);
    EXPECT_EQ(r.offset, 200u);
}

} // namespace bytetaper::pagination
