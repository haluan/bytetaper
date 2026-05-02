// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/cache_policy.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(CachePolicyTest, DisabledByDefault) {
    CachePolicy p{};
    EXPECT_FALSE(p.enabled);
    EXPECT_EQ(validate_cache_policy(p), nullptr);
}

TEST(CachePolicyTest, EnabledL1OnlyValid) {
    CachePolicy p{};
    p.enabled = true;
    p.ttl_seconds = 60;
    p.l1.enabled = true;
    EXPECT_EQ(validate_cache_policy(p), nullptr);
}

TEST(CachePolicyTest, EnabledL1AndL2Valid) {
    CachePolicy p{};
    p.enabled = true;
    p.ttl_seconds = 300;
    p.l1.enabled = true;
    p.l2.enabled = true;
    std::strncpy(p.l2.path, "/tmp/bytetaper_l2", kMaxCachePathLen - 1);
    EXPECT_EQ(validate_cache_policy(p), nullptr);
}

TEST(CachePolicyTest, TtlRequiredWhenEnabled) {
    CachePolicy p{};
    p.enabled = true;
    p.ttl_seconds = 0;
    p.l1.enabled = true;
    EXPECT_NE(validate_cache_policy(p), nullptr);
}

TEST(CachePolicyTest, MissingL2PathInvalid) {
    CachePolicy p{};
    p.enabled = true;
    p.ttl_seconds = 60;
    p.l2.enabled = true;
    // l2.path left empty
    EXPECT_NE(validate_cache_policy(p), nullptr);
}

TEST(CachePolicyTest, AtLeastOneLayerRequired) {
    CachePolicy p{};
    p.enabled = true;
    p.ttl_seconds = 60;
    p.l1.enabled = false;
    p.l2.enabled = false;
    EXPECT_NE(validate_cache_policy(p), nullptr);
}

} // namespace bytetaper::policy
