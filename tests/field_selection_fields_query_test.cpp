// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "field_selection/request_target.h"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace bytetaper::field_selection {

TEST(FieldSelectionFieldsQueryTest, ParsesCommaSeparatedFieldsFromFirstFieldsParameter) {
    struct TestCase {
        const char* raw_query;
        const char* expected[policy::kMaxFields];
        std::size_t expected_count;
    };

    const TestCase cases[] = {
        { "fields=id,name,email", { "id", "name", "email" }, 3 },
        { "page=1&fields=id,name&sort=desc", { "id", "name" }, 2 },
        { "sort=desc&page=1", { nullptr }, 0 },
        { "fields=", { nullptr }, 0 },
        { "fields=,,", { nullptr }, 0 },
        { "a=1&fields=&b=2", { nullptr }, 0 },
        { "fields=id&fields=ignored", { "id" }, 1 },
        { "fields=id,,name,", { "id", "name" }, 2 },
    };

    for (const TestCase& test_case : cases) {
        apg::ApgTransformContext context{};
        std::strncpy(context.raw_query, test_case.raw_query,
                     apg::ApgTransformContext::kRawQueryBufferSize - 1);
        context.raw_query[apg::ApgTransformContext::kRawQueryBufferSize - 1] = '\0';
        context.raw_query_length = std::strlen(context.raw_query);

        ParsedFieldsQuery parsed_fields{};
        EXPECT_TRUE(parse_fields_query_parameter(context, &parsed_fields));
        EXPECT_EQ(parsed_fields.field_count, test_case.expected_count);
        for (std::size_t i = 0; i < test_case.expected_count; ++i) {
            EXPECT_STREQ(parsed_fields.fields[i], test_case.expected[i]);
        }
    }
}

TEST(FieldSelectionFieldsQueryTest, EnforcesCountAndNameBoundsSafely) {
    std::string query = "fields=";
    for (std::size_t i = 0; i < policy::kMaxFields + 4; ++i) {
        if (i > 0) {
            query += ",";
        }
        query += "f";
        query += std::to_string(i);
    }

    const std::string long_name(policy::kMaxFieldNameLen + 16, 'x');
    query += ",";
    query += long_name;

    apg::ApgTransformContext context{};
    std::strncpy(context.raw_query, query.c_str(),
                 apg::ApgTransformContext::kRawQueryBufferSize - 1);
    context.raw_query[apg::ApgTransformContext::kRawQueryBufferSize - 1] = '\0';
    context.raw_query_length = std::strlen(context.raw_query);

    ParsedFieldsQuery parsed_fields{};
    EXPECT_TRUE(parse_fields_query_parameter(context, &parsed_fields));
    EXPECT_EQ(parsed_fields.field_count, policy::kMaxFields);
    for (std::size_t i = 0; i < parsed_fields.field_count; ++i) {
        EXPECT_LT(std::strlen(parsed_fields.fields[i]), policy::kMaxFieldNameLen);
    }
}

TEST(FieldSelectionFieldsQueryTest, ReturnsFalseForNullOutput) {
    apg::ApgTransformContext context{};
    std::strcpy(context.raw_query, "fields=id");
    context.raw_query_length = std::strlen(context.raw_query);

    EXPECT_FALSE(parse_fields_query_parameter(context, nullptr));
}

} // namespace bytetaper::field_selection
