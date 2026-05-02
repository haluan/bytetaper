// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_safety.h"

#include <gtest/gtest.h>

namespace bytetaper::cache {

TEST(CacheAuthSafetyTest, NoHeaders_AllowCache) {
    // No auth headers, no private cache opt-in -> Should NOT bypass (allow cache)
    EXPECT_FALSE(cache_auth_bypass(false, false, false));
}

TEST(CacheAuthSafetyTest, AuthorizationHeader_NoCachePrivate_Bypass) {
    // Authorization header present, no private cache opt-in -> Should bypass
    EXPECT_TRUE(cache_auth_bypass(true, false, false));
}

TEST(CacheAuthSafetyTest, CookieHeader_NoCachePrivate_Bypass) {
    // Cookie header present, no private cache opt-in -> Should bypass
    EXPECT_TRUE(cache_auth_bypass(false, true, false));
}

TEST(CacheAuthSafetyTest, AuthorizationHeader_PrivateCacheEnabled_Allow) {
    // Authorization header present, BUT private cache opt-in -> Should NOT bypass (allow cache)
    EXPECT_FALSE(cache_auth_bypass(true, false, true));
}

TEST(CacheAuthSafetyTest, BothHeaders_PrivateCacheEnabled_Allow) {
    // Both headers present, BUT private cache opt-in -> Should NOT bypass (allow cache)
    EXPECT_FALSE(cache_auth_bypass(true, true, true));
}

} // namespace bytetaper::cache
