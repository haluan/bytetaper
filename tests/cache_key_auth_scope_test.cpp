// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_key.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::cache {

static CacheKeyInput make_private_input(const char* scope) {
    CacheKeyInput in{};
    in.method = policy::HttpMethod::Get;
    in.route_id = "api-v1";
    in.path = "/api/items";
    in.policy_version = "v1";
    in.private_cache = true;
    in.auth_scope = scope;
    return in;
}

TEST(CacheKeyAuthScopeTest, DifferentScopes_DifferentKeys) {
    char key1[512]{}, key2[512]{};
    auto in1 = make_private_input("user-abc");
    auto in2 = make_private_input("user-xyz");
    ASSERT_TRUE(build_cache_key(in1, key1, sizeof(key1)));
    ASSERT_TRUE(build_cache_key(in2, key2, sizeof(key2)));
    EXPECT_STRNE(key1, key2);
}

TEST(CacheKeyAuthScopeTest, SameScope_SameKey) {
    char key1[512]{}, key2[512]{};
    auto in1 = make_private_input("user-abc");
    auto in2 = make_private_input("user-abc");
    ASSERT_TRUE(build_cache_key(in1, key1, sizeof(key1)));
    ASSERT_TRUE(build_cache_key(in2, key2, sizeof(key2)));
    EXPECT_STREQ(key1, key2);
}

TEST(CacheKeyAuthScopeTest, MissingScope_ReturnsFalse) {
    char key[512]{};
    auto in = make_private_input(nullptr);
    EXPECT_FALSE(build_cache_key(in, key, sizeof(key)));
}

TEST(CacheKeyAuthScopeTest, PublicCache_NoAuthSegment) {
    char key[512]{};
    CacheKeyInput in{};
    in.method = policy::HttpMethod::Get;
    in.route_id = "api-v1";
    in.path = "/api/items";
    in.policy_version = "v1";
    in.private_cache = false;
    ASSERT_TRUE(build_cache_key(in, key, sizeof(key)));
    EXPECT_EQ(std::strstr(key, "scope:"), nullptr);
}

TEST(CacheKeyAuthScopeTest, PublicAndPrivateSamePath_DifferentKeys) {
    char pub_key[512]{}, priv_key[512]{};
    CacheKeyInput pub_in{};
    pub_in.method = policy::HttpMethod::Get;
    pub_in.route_id = "api-v1";
    pub_in.path = "/api/items";
    pub_in.policy_version = "v1";
    pub_in.private_cache = false;

    auto priv_in = make_private_input("user-abc");

    ASSERT_TRUE(build_cache_key(pub_in, pub_key, sizeof(pub_key)));
    ASSERT_TRUE(build_cache_key(priv_in, priv_key, sizeof(priv_key)));
    EXPECT_STRNE(pub_key, priv_key);
}

} // namespace bytetaper::cache
