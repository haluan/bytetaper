// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_policy.h"
#include "policy/yaml_loader.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(YamlLoaderTest, ValidYamlSingleRoutePrefixDisabled) {
    const char* yaml = R"(
routes:
  - id: "test-id"
    match:
      kind: "prefix"
      prefix: "/test/"
    mutation: "disabled"
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.count, 1u);
    EXPECT_STREQ(result.policies[0].route_id, "test-id");
    EXPECT_EQ(result.policies[0].match_kind, RouteMatchKind::Prefix);
    EXPECT_STREQ(result.policies[0].match_prefix, "/test/");
    EXPECT_EQ(result.policies[0].mutation, MutationMode::Disabled);
}

TEST(YamlLoaderTest, ValidYamlMutationHeadersOnly) {
    const char* yaml = R"(
routes:
  - id: "test-id"
    match:
      kind: "prefix"
      prefix: "/"
    mutation: "headers_only"
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].mutation, MutationMode::HeadersOnly);
}

TEST(YamlLoaderTest, ValidYamlMutationFull) {
    const char* yaml = R"(
routes:
  - id: "test-id"
    match:
      kind: "prefix"
      prefix: "/"
    mutation: "full"
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].mutation, MutationMode::Full);
}

TEST(YamlLoaderTest, ValidYamlExactMatchKind) {
    const char* yaml = R"(
routes:
  - id: "test-id"
    match:
      kind: "exact"
      prefix: "/exact"
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].match_kind, RouteMatchKind::Exact);
}

TEST(YamlLoaderTest, ValidYamlMultipleRoutes) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/1" }
  - id: "r2"
    match: { kind: "prefix", prefix: "/2" }
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.count, 2u);
    EXPECT_STREQ(result.policies[1].route_id, "r2");
}

TEST(YamlLoaderTest, InvalidYamlMissingRoutes) {
    const char* yaml = "{}";
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error, nullptr);
}

TEST(YamlLoaderTest, InvalidYamlMissingId) {
    const char* yaml = R"(
routes:
  - match: { kind: "prefix", prefix: "/1" }
)";
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
}

TEST(YamlLoaderTest, InvalidYamlMissingMatchPrefix) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix" }
)";
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
}

TEST(YamlLoaderTest, InvalidYamlBadMutationValue) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
    mutation: "superfast"
)";
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
}

TEST(YamlLoaderTest, InvalidYamlBadMatchKind) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "wildcard", prefix: "/" }
)";
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
}

TEST(YamlLoaderTest, NullContentReturnsError) {
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(nullptr, &result));
}

TEST(YamlLoaderTest, DefaultMutationIsDisabled) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].mutation, MutationMode::Disabled);
}

TEST(YamlLoaderTest, RoutePolicyPassesValidation) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/api" }
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_TRUE(validate_route_policy(result.policies[0], nullptr));
}

} // namespace bytetaper::policy
