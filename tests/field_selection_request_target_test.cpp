// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "field_selection/request_target.h"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace bytetaper::field_selection {

TEST(FieldSelectionRequestTargetTest, ExtractsPathAndQueryForCommonCases) {
    struct TestCase {
        const char* request_target;
        const char* expected_path;
        const char* expected_query;
    };

    const TestCase cases[] = {
        { "/users/1?fields=id,name,email", "/users/1", "fields=id,name,email" },
        { "/health", "/health", "" },
        { "?fields=id", "", "fields=id" },
        { "", "", "" },
    };

    for (const TestCase& test_case : cases) {
        apg::ApgTransformContext context{};
        EXPECT_TRUE(extract_raw_path_and_query(test_case.request_target, &context));
        EXPECT_STREQ(context.raw_path, test_case.expected_path);
        EXPECT_STREQ(context.raw_query, test_case.expected_query);
        EXPECT_EQ(context.raw_path_length, std::strlen(test_case.expected_path));
        EXPECT_EQ(context.raw_query_length, std::strlen(test_case.expected_query));
    }
}

TEST(FieldSelectionRequestTargetTest, HandlesNullInputAsEmpty) {
    apg::ApgTransformContext context{};
    context.raw_path[0] = 'x';
    context.raw_query[0] = 'y';

    EXPECT_TRUE(extract_raw_path_and_query(nullptr, &context));
    EXPECT_STREQ(context.raw_path, "");
    EXPECT_STREQ(context.raw_query, "");
    EXPECT_EQ(context.raw_path_length, 0u);
    EXPECT_EQ(context.raw_query_length, 0u);
}

TEST(FieldSelectionRequestTargetTest, ReturnsFalseForNullContext) {
    EXPECT_FALSE(extract_raw_path_and_query("/users", nullptr));
}

TEST(FieldSelectionRequestTargetTest, TruncatesPathAndQuerySafely) {
    const std::string long_path(apg::ApgTransformContext::kRawPathBufferSize + 32, 'p');
    const std::string long_query(apg::ApgTransformContext::kRawQueryBufferSize + 32, 'q');
    const std::string request_target = long_path + "?" + long_query;

    apg::ApgTransformContext context{};
    EXPECT_TRUE(extract_raw_path_and_query(request_target.c_str(), &context));

    EXPECT_EQ(context.raw_path_length, apg::ApgTransformContext::kRawPathBufferSize - 1);
    EXPECT_EQ(context.raw_query_length, apg::ApgTransformContext::kRawQueryBufferSize - 1);
    EXPECT_EQ(context.raw_path[context.raw_path_length], '\0');
    EXPECT_EQ(context.raw_query[context.raw_query_length], '\0');
}

} // namespace bytetaper::field_selection
