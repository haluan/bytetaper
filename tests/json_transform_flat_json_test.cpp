// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "json_transform/content_type.h"

#include <cstdio>
#include <cstring>

#include <gtest/gtest.h>

namespace bytetaper::json_transform {

namespace {

void expect_field(const ParsedFlatJsonObject& object, std::size_t index, const char* key,
                  const char* value_slice) {
    ASSERT_LT(index, object.field_count);
    EXPECT_STREQ(object.fields[index].key, key);
    const std::size_t begin = object.fields[index].value_begin;
    const std::size_t end = object.fields[index].value_end;
    ASSERT_LE(begin, end);
    const std::size_t expected_length = std::strlen(value_slice);
    ASSERT_EQ(end - begin, expected_length);
    EXPECT_EQ(std::strncmp(object.source + begin, value_slice, expected_length), 0);
}

} // namespace

TEST(JsonTransformFlatJsonTest, ParsesFlatObjectWithDeterministicValueSpans) {
    const char* body = R"({"id":1,"name":"Andi","active":true,"quota":null})";
    ParsedFlatJsonObject parsed{};

    const FlatJsonParseStatus status =
        parse_flat_json_object(body, JsonResponseKind::EligibleJson, &parsed);
    ASSERT_EQ(status, FlatJsonParseStatus::Ok);
    ASSERT_EQ(parsed.field_count, 4u);
    ASSERT_EQ(parsed.source, body);

    expect_field(parsed, 0, "id", "1");
    expect_field(parsed, 1, "name", "\"Andi\"");
    expect_field(parsed, 2, "active", "true");
    expect_field(parsed, 3, "quota", "null");
}

TEST(JsonTransformFlatJsonTest, ParsesWithWhitespace) {
    const char* body = R"({ "id" : 1 , "name" : "A" })";
    ParsedFlatJsonObject parsed{};

    ASSERT_EQ(parse_flat_json_object(body, JsonResponseKind::EligibleJson, &parsed),
              FlatJsonParseStatus::Ok);
    ASSERT_EQ(parsed.field_count, 2u);
    expect_field(parsed, 0, "id", "1");
    expect_field(parsed, 1, "name", "\"A\"");
}

TEST(JsonTransformFlatJsonTest, SkipsUnsupportedResponseKind) {
    ParsedFlatJsonObject parsed{};
    ASSERT_EQ(parse_flat_json_object("{}", JsonResponseKind::SkipUnsupported, &parsed),
              FlatJsonParseStatus::SkipUnsupported);
    EXPECT_EQ(parsed.field_count, 0u);
}

TEST(JsonTransformFlatJsonTest, SkipsUnsupportedStructure) {
    ParsedFlatJsonObject parsed{};
    ASSERT_EQ(parse_flat_json_object(R"([{"id":1}])", JsonResponseKind::EligibleJson, &parsed),
              FlatJsonParseStatus::SkipUnsupported);

    ASSERT_EQ(
        parse_flat_json_object(R"({"id":{"nested":1}})", JsonResponseKind::EligibleJson, &parsed),
        FlatJsonParseStatus::SkipUnsupported);
}

TEST(JsonTransformFlatJsonTest, ReturnsInvalidJsonForMalformedBody) {
    ParsedFlatJsonObject parsed{};
    const char* cases[] = {
        R"({"id":1)", R"({"id" 1})", R"({"id":"x})", R"({"id":tru})", R"({"id":1,})",
    };

    for (const char* body : cases) {
        EXPECT_EQ(parse_flat_json_object(body, JsonResponseKind::EligibleJson, &parsed),
                  FlatJsonParseStatus::InvalidJson);
    }
}

TEST(JsonTransformFlatJsonTest, ReturnsInvalidInputForInvalidCallUsage) {
    EXPECT_EQ(parse_flat_json_object("{}", JsonResponseKind::EligibleJson, nullptr),
              FlatJsonParseStatus::InvalidInput);

    ParsedFlatJsonObject parsed{};
    EXPECT_EQ(parse_flat_json_object(nullptr, JsonResponseKind::EligibleJson, &parsed),
              FlatJsonParseStatus::InvalidInput);
}

TEST(JsonTransformFlatJsonTest, CapsStoredFieldsAtPolicyBoundary) {
    char body[1024] = {};
    std::size_t offset = 0;
    body[offset++] = '{';
    for (std::size_t i = 0; i < policy::kMaxFields + 2; ++i) {
        if (i > 0) {
            body[offset++] = ',';
        }
        const int written =
            std::snprintf(body + offset, sizeof(body) - offset, "\"k%zu\":%zu", i, i);
        ASSERT_GT(written, 0);
        offset += static_cast<std::size_t>(written);
        ASSERT_LT(offset, sizeof(body));
    }
    body[offset++] = '}';
    body[offset] = '\0';

    ParsedFlatJsonObject parsed{};
    ASSERT_EQ(parse_flat_json_object(body, JsonResponseKind::EligibleJson, &parsed),
              FlatJsonParseStatus::Ok);
    EXPECT_EQ(parsed.field_count, policy::kMaxFields);
    EXPECT_STREQ(parsed.fields[0].key, "k0");
}

} // namespace bytetaper::json_transform
