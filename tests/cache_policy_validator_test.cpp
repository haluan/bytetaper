// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/cache_policy_validator.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::policy {

static CachePolicy make_valid_public_policy() {
    CachePolicy p{};
    p.enabled = true;
    p.ttl_seconds = 60;
    p.l1.enabled = true;
    return p;
}

TEST(CachePolicyValidatorTest, ValidPublicGetPolicy_Passes) {
    EXPECT_EQ(validate_cache_policy_safe(make_valid_public_policy(), HttpMethod::Get), nullptr);
}

TEST(CachePolicyValidatorTest, InvalidTtl_Fails) {
    CachePolicy p = make_valid_public_policy();
    p.ttl_seconds = 0;
    EXPECT_NE(validate_cache_policy_safe(p, HttpMethod::Get), nullptr);
}

TEST(CachePolicyValidatorTest, MissingL2Path_Fails) {
    CachePolicy p = make_valid_public_policy();
    p.l2.enabled = true;
    // l2.path left empty
    EXPECT_NE(validate_cache_policy_safe(p, HttpMethod::Get), nullptr);
}

TEST(CachePolicyValidatorTest, PrivateCacheWithoutScopeHeader_Fails) {
    CachePolicy p = make_valid_public_policy();
    p.private_cache = true;
    // auth_scope_header left empty
    EXPECT_NE(validate_cache_policy_safe(p, HttpMethod::Get), nullptr);
}

TEST(CachePolicyValidatorTest, NonGetRoute_Fails) {
    EXPECT_NE(validate_cache_policy_safe(make_valid_public_policy(), HttpMethod::Post), nullptr);
}

} // namespace bytetaper::policy
