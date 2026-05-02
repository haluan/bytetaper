// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::cache {

TEST(CacheEntryTest, DefaultConstruction) {
    CacheEntry e{};
    EXPECT_EQ(e.key[0], '\0');
    EXPECT_EQ(e.status_code, 0u);
    EXPECT_EQ(e.content_type[0], '\0');
    EXPECT_EQ(e.body, nullptr);
    EXPECT_EQ(e.body_len, 0u);
    EXPECT_EQ(e.created_at_epoch_ms, 0);
    EXPECT_EQ(e.expires_at_epoch_ms, 0);
}

TEST(CacheEntryTest, NoExpiryWhenZero) {
    CacheEntry e{};
    e.expires_at_epoch_ms = 0;
    EXPECT_FALSE(is_expired(e, 9999999999LL));
}

TEST(CacheEntryTest, NotExpiredBeforeDeadline) {
    CacheEntry e{};
    e.expires_at_epoch_ms = 2000;
    EXPECT_FALSE(is_expired(e, 1000));
}

TEST(CacheEntryTest, ExpiredAtDeadline) {
    CacheEntry e{};
    e.expires_at_epoch_ms = 1000;
    EXPECT_TRUE(is_expired(e, 1000));
}

TEST(CacheEntryTest, ExpiredAfterDeadline) {
    CacheEntry e{};
    e.expires_at_epoch_ms = 1000;
    EXPECT_TRUE(is_expired(e, 2000));
}

TEST(CacheEntryTest, EmptyBodyAllowed) {
    CacheEntry e{};
    e.status_code = 200;
    e.body = nullptr;
    e.body_len = 0;
    EXPECT_EQ(e.body, nullptr);
    EXPECT_EQ(e.body_len, 0u);
}

TEST(CacheEntryTest, StoresStatusCodeAndContentType) {
    CacheEntry e{};
    e.status_code = 200;
    std::strncpy(e.content_type, "application/json", kCacheContentTypeMaxLen - 1);
    EXPECT_EQ(e.status_code, 200u);
    EXPECT_STREQ(e.content_type, "application/json");
}

} // namespace bytetaper::cache
