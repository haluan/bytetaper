// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/pagination_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

static PaginationPolicy make_valid() {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::LimitOffset;
    p.default_limit = 20;
    p.max_limit = 500;
    // limit_param / offset_param default to "limit" / "offset"
    return p;
}

TEST(PaginationPolicyValidatorTest, ValidLimitOffsetPolicy_Passes) {
    auto p = make_valid();
    EXPECT_EQ(validate_pagination_policy_safe(p), nullptr);
}

TEST(PaginationPolicyValidatorTest, DisabledPolicy_Passes) {
    PaginationPolicy p{};
    EXPECT_EQ(validate_pagination_policy_safe(p), nullptr);
}

TEST(PaginationPolicyValidatorTest, ZeroDefaultLimit_Fails) {
    auto p = make_valid();
    p.default_limit = 0;
    EXPECT_NE(validate_pagination_policy_safe(p), nullptr);
}

TEST(PaginationPolicyValidatorTest, ZeroMaxLimit_Fails) {
    auto p = make_valid();
    p.max_limit = 0;
    EXPECT_NE(validate_pagination_policy_safe(p), nullptr);
}

TEST(PaginationPolicyValidatorTest, DefaultLimitExceedsMaxLimit_Fails) {
    auto p = make_valid();
    p.default_limit = 600;
    p.max_limit = 500;
    EXPECT_NE(validate_pagination_policy_safe(p), nullptr);
}

TEST(PaginationPolicyValidatorTest, EmptyLimitParam_Fails) {
    auto p = make_valid();
    p.limit_param[0] = '\0';
    EXPECT_NE(validate_pagination_policy_safe(p), nullptr);
}

TEST(PaginationPolicyValidatorTest, UpstreamUnsupported_StillValid) {
    // Mutation is still applied even when upstream_supports_pagination=false.
    // Safe validator must not reject this config.
    auto p = make_valid();
    p.upstream_supports_pagination = false;
    EXPECT_EQ(validate_pagination_policy_safe(p), nullptr);
}

} // namespace bytetaper::policy
