// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/pagination_policy.h"

#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(PaginationPolicyTest, DisabledByDefault) {
    PaginationPolicy p{};
    EXPECT_FALSE(p.enabled);
    EXPECT_EQ(validate_pagination_policy(p), nullptr);
}

TEST(PaginationPolicyTest, LimitOffsetModeValid) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::LimitOffset;
    p.default_limit = 20;
    p.max_limit = 100;
    EXPECT_EQ(validate_pagination_policy(p), nullptr);
}

TEST(PaginationPolicyTest, MissingMode_Fails) {
    PaginationPolicy p{};
    p.enabled = true;
    p.default_limit = 20;
    // mode left as None
    EXPECT_NE(validate_pagination_policy(p), nullptr);
}

TEST(PaginationPolicyTest, MissingDefaultLimit_Fails) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::LimitOffset;
    p.default_limit = 0;
    EXPECT_NE(validate_pagination_policy(p), nullptr);
}

TEST(PaginationPolicyTest, MaxLimitBelowDefault_Fails) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::LimitOffset;
    p.default_limit = 50;
    p.max_limit = 10; // less than default
    EXPECT_NE(validate_pagination_policy(p), nullptr);
}

TEST(PaginationPolicyTest, UpstreamSupportsFlagStored) {
    PaginationPolicy p{};
    p.enabled = true;
    p.mode = PaginationMode::LimitOffset;
    p.default_limit = 20;
    p.upstream_supports_pagination = true;
    EXPECT_EQ(validate_pagination_policy(p), nullptr);
    EXPECT_TRUE(p.upstream_supports_pagination);
}

} // namespace bytetaper::policy
