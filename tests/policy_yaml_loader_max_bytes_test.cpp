// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_policy.h"
#include "policy/yaml_loader.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(YamlLoaderMaxBytesTest, ValidYamlOneMegabyte) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
    max_response_bytes: 1048576
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].max_response_bytes, 1048576u);
}

TEST(YamlLoaderMaxBytesTest, ValidYamlZeroIsUnlimited) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
    max_response_bytes: 0
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].max_response_bytes, 0u);
}

TEST(YamlLoaderMaxBytesTest, ValidYamlAbsent) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
)";
    PolicyFileResult result{};
    EXPECT_TRUE(load_policy_from_string(yaml, &result));
    EXPECT_EQ(result.policies[0].max_response_bytes, 0u);
}

TEST(YamlLoaderMaxBytesTest, InvalidYamlExceedsUint32) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
    max_response_bytes: 9999999999
)";
    PolicyFileResult result{};
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
}

} // namespace bytetaper::policy
