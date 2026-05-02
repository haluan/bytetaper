// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/l2_disk_cache.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::cache {

static const char* kTestDbPath = "/tmp/bytetaper_l2_test";

class L2RocksDbCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        l2_destroy(kTestDbPath);
        cache_ = l2_open(kTestDbPath);
        ASSERT_NE(cache_, nullptr);
    }

    void TearDown() override {
        l2_close(&cache_);
        l2_destroy(kTestDbPath);
    }

    L2DiskCache* cache_ = nullptr;
};

TEST_F(L2RocksDbCacheTest, PutGetReturnsSameEntry) {
    CacheEntry e{};
    std::strncpy(e.key, "GET|rt1|/api/items||v1", kCacheKeyMaxLen - 1);
    e.status_code = 200;
    std::strncpy(e.content_type, "application/json", kCacheContentTypeMaxLen - 1);
    const char body[] = "{\"id\": 1, \"name\": \"item1\"}";
    e.body = body;
    e.body_len = std::strlen(body);
    e.created_at_epoch_ms = 1000;
    e.expires_at_epoch_ms = 9999999;

    EXPECT_TRUE(l2_put(cache_, e));

    CacheEntry out{};
    char body_buf[128] = {};
    EXPECT_TRUE(l2_get(cache_, e.key, 0, &out, body_buf, sizeof(body_buf)));

    EXPECT_EQ(out.status_code, e.status_code);
    EXPECT_STREQ(out.content_type, e.content_type);
    EXPECT_EQ(out.body_len, e.body_len);
    EXPECT_EQ(std::memcmp(out.body, e.body, e.body_len), 0);
    EXPECT_EQ(out.created_at_epoch_ms, e.created_at_epoch_ms);
    EXPECT_EQ(out.expires_at_epoch_ms, e.expires_at_epoch_ms);
}

TEST_F(L2RocksDbCacheTest, UnknownKeyMisses) {
    CacheEntry out{};
    char body_buf[64] = {};
    EXPECT_FALSE(l2_get(cache_, "non_existent_key", 0, &out, body_buf, sizeof(body_buf)));
}

TEST_F(L2RocksDbCacheTest, RemoveDeletesEntry) {
    CacheEntry e{};
    std::strncpy(e.key, "test_key", kCacheKeyMaxLen - 1);
    e.body = "data";
    e.body_len = 4;
    e.expires_at_epoch_ms = 9999999;

    EXPECT_TRUE(l2_put(cache_, e));
    EXPECT_TRUE(l2_remove(cache_, e.key));

    CacheEntry out{};
    char body_buf[64] = {};
    EXPECT_FALSE(l2_get(cache_, e.key, 0, &out, body_buf, sizeof(body_buf)));
}

TEST_F(L2RocksDbCacheTest, ExpiredEntryMisses) {
    CacheEntry e{};
    std::strncpy(e.key, "expired_key", kCacheKeyMaxLen - 1);
    e.body = "old_data";
    e.body_len = 8;
    e.expires_at_epoch_ms = 1000; // Expired at 1000ms

    EXPECT_TRUE(l2_put(cache_, e));

    CacheEntry out{};
    char body_buf[64] = {};
    // current time is 2000ms, entry expired at 1000ms
    EXPECT_FALSE(l2_get(cache_, e.key, 2000, &out, body_buf, sizeof(body_buf)));
}

TEST_F(L2RocksDbCacheTest, BufferTooSmallReturnsFalse) {
    CacheEntry e{};
    std::strncpy(e.key, "large_key", kCacheKeyMaxLen - 1);
    const char body[] = "this is a long body";
    e.body = body;
    e.body_len = std::strlen(body);
    e.expires_at_epoch_ms = 9999999;

    EXPECT_TRUE(l2_put(cache_, e));

    CacheEntry out{};
    char body_buf[5] = {}; // way too small
    EXPECT_FALSE(l2_get(cache_, e.key, 0, &out, body_buf, sizeof(body_buf)));
}

} // namespace bytetaper::cache
