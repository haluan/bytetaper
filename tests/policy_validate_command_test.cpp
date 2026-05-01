// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_policy.h"
#include "policy/yaml_loader.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(ValidateCommandLogicTest, ValidFileLoadsAndAllRoutesPass) {
    const char* yaml = R"(
routes:
  - id: "r1"
    match: { kind: "prefix", prefix: "/" }
)";
    PolicyFileResult result{};
    ASSERT_TRUE(load_policy_from_string(yaml, &result));
    ASSERT_GT(result.count, 0u);

    for (std::size_t i = 0; i < result.count; ++i) {
        const char* reason = nullptr;
        EXPECT_TRUE(validate_route_policy(result.policies[i], &reason));
    }
}

TEST(ValidateCommandLogicTest, InvalidYamlReturnsTwoPath) {
    const char* yaml = "!!! invalid yaml !!!";
    PolicyFileResult result{};
    // simulate exit code 2 path
    EXPECT_FALSE(load_policy_from_string(yaml, &result));
}

TEST(ValidateCommandLogicTest, InvalidRoutePolicyReturnsThreePath) {
    const char* yaml = R"(
routes:
  - id: ""
    match: { kind: "prefix", prefix: "/" }
)";
    PolicyFileResult result{};
    ASSERT_TRUE(load_policy_from_string(yaml, &result));
    ASSERT_EQ(result.count, 1u);

    // simulate exit code 3 path
    const char* reason = nullptr;
    EXPECT_FALSE(validate_route_policy(result.policies[0], &reason));
}

} // namespace bytetaper::policy
