// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/pagination_policy.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(PaginationCursorPlaceholderTest, CursorMode_Represented) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::Cursor; // must compile
    EXPECT_EQ(p.mode, PaginationMode::Cursor);
}

TEST(PaginationCursorPlaceholderTest, CursorMode_ReturnsNotImplemented) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::Cursor;
    const char* err = validate_pagination_policy(p);
    ASSERT_NE(err, nullptr);
    EXPECT_STREQ(err, "cursor_mode_not_implemented");
}

TEST(PaginationCursorPlaceholderTest, CursorMode_Disabled_Valid) {
    PaginationPolicy p{};
    p.enabled = false;
    p.mode = PaginationMode::Cursor;
    EXPECT_EQ(validate_pagination_policy(p), nullptr);
}

TEST(PaginationCursorPlaceholderTest, LimitOffsetMode_Unaffected) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::LimitOffset;
    p.default_limit = 20;
    p.max_limit = 100;
    EXPECT_EQ(validate_pagination_policy(p), nullptr);
}

} // namespace bytetaper::policy
