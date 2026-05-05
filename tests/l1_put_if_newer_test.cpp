// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/l1_cache.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>

namespace bytetaper::cache {

class L1PutIfNewerTest : public ::testing::Test {
protected:
    void SetUp() override {
        l1_cache = std::make_unique<L1Cache>();
        l1_init(l1_cache.get());
    }
    std::unique_ptr<L1Cache> l1_cache;
};

TEST_F(L1PutIfNewerTest, EmptyL1PromotesUnconditionally) {
    CacheEntry entry{};
    std::strcpy(entry.key, "key1");
    std::strcpy(entry.content_type, "text/plain");
    entry.created_at_epoch_ms = 1000;
    entry.body = "data";
    entry.body_len = 4;

    l1_put_if_newer(l1_cache.get(), entry);

    CacheEntry out{};
    char body_buf[kL1MaxBodySize] = {};
    EXPECT_TRUE(l1_get(l1_cache.get(), "key1", 0, &out, body_buf, sizeof(body_buf)));
    EXPECT_EQ(out.created_at_epoch_ms, 1000);
}

TEST_F(L1PutIfNewerTest, NewerExistingRejectsStaleIncoming) {
    CacheEntry existing{};
    std::strcpy(existing.key, "key1");
    existing.created_at_epoch_ms = 2000;
    l1_put(l1_cache.get(), existing);

    CacheEntry stale{};
    std::strcpy(stale.key, "key1");
    stale.created_at_epoch_ms = 1000;
    stale.body = "stale";
    stale.body_len = 5;

    l1_put_if_newer(l1_cache.get(), stale);

    CacheEntry out{};
    char body_buf[kL1MaxBodySize] = {};
    EXPECT_TRUE(l1_get(l1_cache.get(), "key1", 0, &out, body_buf, sizeof(body_buf)));
    EXPECT_EQ(out.created_at_epoch_ms, 2000); // Still 2000
    EXPECT_EQ(out.body_len, 0);               // Not the "stale" body
}

TEST_F(L1PutIfNewerTest, OlderExistingAcceptsNewerIncoming) {
    CacheEntry existing{};
    std::strcpy(existing.key, "key1");
    existing.created_at_epoch_ms = 1000;
    l1_put(l1_cache.get(), existing);

    CacheEntry newer{};
    std::strcpy(newer.key, "key1");
    newer.created_at_epoch_ms = 2000;
    newer.body = "newer";
    newer.body_len = 5;

    l1_put_if_newer(l1_cache.get(), newer);

    CacheEntry out{};
    char body_buf[kL1MaxBodySize] = {};
    EXPECT_TRUE(l1_get(l1_cache.get(), "key1", 0, &out, body_buf, sizeof(body_buf)));
    EXPECT_EQ(out.created_at_epoch_ms, 2000);
    EXPECT_STREQ(static_cast<const char*>(out.body), "newer");
}

TEST_F(L1PutIfNewerTest, SameTimestampPromotes) {
    CacheEntry existing{};
    std::strcpy(existing.key, "key1");
    existing.created_at_epoch_ms = 1000;
    l1_put(l1_cache.get(), existing);

    CacheEntry same{};
    std::strcpy(same.key, "key1");
    same.created_at_epoch_ms = 1000;
    same.body = "same";
    same.body_len = 4;

    l1_put_if_newer(l1_cache.get(), same);

    CacheEntry out{};
    char body_buf[kL1MaxBodySize] = {};
    EXPECT_TRUE(l1_get(l1_cache.get(), "key1", 0, &out, body_buf, sizeof(body_buf)));
    EXPECT_STREQ(static_cast<const char*>(out.body), "same");
}

} // namespace bytetaper::cache
