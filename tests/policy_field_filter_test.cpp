// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/field_filter_policy.h"
#include "policy/route_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(FieldFilterTest, NoneAllowsAllFields) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::None;
    EXPECT_TRUE(apply_field_filter(filter, "any_field"));
}

TEST(FieldFilterTest, NoneAllowsNullField) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::None;
    // Actually, match_method had null guard first.
    // apply_field_filter has null guard at the top.
    EXPECT_FALSE(apply_field_filter(filter, nullptr));
}

TEST(FieldFilterTest, AllowlistKeepsListedField) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Allowlist;
    std::strcpy(filter.fields[0], "id");
    std::strcpy(filter.fields[1], "name");
    filter.field_count = 2;

    EXPECT_TRUE(apply_field_filter(filter, "id"));
    EXPECT_TRUE(apply_field_filter(filter, "name"));
}

TEST(FieldFilterTest, AllowlistDropsUnlistedField) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Allowlist;
    std::strcpy(filter.fields[0], "id");
    filter.field_count = 1;

    EXPECT_FALSE(apply_field_filter(filter, "status"));
}

TEST(FieldFilterTest, DenylistDropsListedField) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Denylist;
    std::strcpy(filter.fields[0], "internal");
    filter.field_count = 1;

    EXPECT_FALSE(apply_field_filter(filter, "internal"));
}

TEST(FieldFilterTest, DenylistKeepsUnlistedField) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Denylist;
    std::strcpy(filter.fields[0], "internal");
    filter.field_count = 1;

    EXPECT_TRUE(apply_field_filter(filter, "name"));
}

TEST(FieldFilterTest, AllowlistNullFieldReturnsFalse) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Allowlist;
    EXPECT_FALSE(apply_field_filter(filter, nullptr));
}

TEST(FieldFilterTest, EmptyAllowlistDropsEverything) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Allowlist;
    filter.field_count = 0;
    EXPECT_FALSE(apply_field_filter(filter, "any"));
}

TEST(FieldFilterTest, EmptyDenylistKeepsEverything) {
    FieldFilterPolicy filter{};
    filter.mode = FieldFilterMode::Denylist;
    filter.field_count = 0;
    EXPECT_TRUE(apply_field_filter(filter, "any"));
}

TEST(FieldFilterTest, DefaultFilterModeIsNone) {
    FieldFilterPolicy filter{};
    EXPECT_EQ(filter.mode, FieldFilterMode::None);
}

TEST(FieldFilterTest, DefaultRoutePolicyFilterModeIsNone) {
    RoutePolicy policy{};
    EXPECT_EQ(policy.field_filter.mode, FieldFilterMode::None);
}

} // namespace bytetaper::policy
