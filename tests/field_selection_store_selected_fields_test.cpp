// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "field_selection/request_target.h"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace bytetaper::field_selection {

TEST(FieldSelectionStoreSelectedFieldsTest, StoresSelectedFieldsFromRawQuery) {
    struct TestCase {
        const char* raw_query;
        const char* expected_fields[policy::kMaxFields];
        std::size_t expected_count;
    };

    const TestCase cases[] = {
        { "fields=id,name,email", { "id", "name", "email" }, 3 },
        { "page=1&fields=id,name&sort=desc", { "id", "name" }, 2 },
        { "sort=desc&page=1", { nullptr }, 0 },
        { "fields=", { nullptr }, 0 },
        { "fields=id&fields=ignored", { "id" }, 1 },
    };

    for (const TestCase& test_case : cases) {
        apg::ApgTransformContext context{};
        std::strncpy(context.raw_query, test_case.raw_query,
                     apg::ApgTransformContext::kRawQueryBufferSize - 1);
        context.raw_query[apg::ApgTransformContext::kRawQueryBufferSize - 1] = '\0';
        context.raw_query_length = std::strlen(context.raw_query);

        EXPECT_TRUE(parse_and_store_selected_fields(&context));
        EXPECT_EQ(context.selected_field_count, test_case.expected_count);
        for (std::size_t i = 0; i < test_case.expected_count; ++i) {
            EXPECT_STREQ(context.selected_fields[i], test_case.expected_fields[i]);
        }
    }
}

TEST(FieldSelectionStoreSelectedFieldsTest, OverwritesPreviousSelections) {
    apg::ApgTransformContext context{};
    std::strcpy(context.raw_query, "fields=id,name");
    context.raw_query_length = std::strlen(context.raw_query);
    ASSERT_TRUE(parse_and_store_selected_fields(&context));
    ASSERT_EQ(context.selected_field_count, 2u);
    ASSERT_STREQ(context.selected_fields[0], "id");
    ASSERT_STREQ(context.selected_fields[1], "name");

    std::strcpy(context.raw_query, "page=1");
    context.raw_query_length = std::strlen(context.raw_query);
    EXPECT_TRUE(parse_and_store_selected_fields(&context));
    EXPECT_EQ(context.selected_field_count, 0u);
    EXPECT_STREQ(context.selected_fields[0], "");
    EXPECT_STREQ(context.selected_fields[1], "");
}

TEST(FieldSelectionStoreSelectedFieldsTest, KeepsStorageBoundedAndNullTerminated) {
    std::string query = "fields=";
    for (std::size_t i = 0; i < policy::kMaxFields + 8; ++i) {
        if (i > 0) {
            query += ",";
        }
        query += "f";
        query += std::to_string(i);
    }
    query += ",";
    query += std::string(policy::kMaxFieldNameLen + 24, 'x');

    apg::ApgTransformContext context{};
    std::strncpy(context.raw_query, query.c_str(),
                 apg::ApgTransformContext::kRawQueryBufferSize - 1);
    context.raw_query[apg::ApgTransformContext::kRawQueryBufferSize - 1] = '\0';
    context.raw_query_length = std::strlen(context.raw_query);

    EXPECT_TRUE(parse_and_store_selected_fields(&context));
    EXPECT_EQ(context.selected_field_count, policy::kMaxFields);
    for (std::size_t i = 0; i < context.selected_field_count; ++i) {
        EXPECT_LT(std::strlen(context.selected_fields[i]), policy::kMaxFieldNameLen);
    }
}

TEST(FieldSelectionStoreSelectedFieldsTest, ReturnsFalseForNullContext) {
    EXPECT_FALSE(parse_and_store_selected_fields(nullptr));
}

} // namespace bytetaper::field_selection
