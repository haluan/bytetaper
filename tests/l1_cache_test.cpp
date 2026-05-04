// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/l1_cache.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace bytetaper::cache {

static CacheEntry make_entry(const char* key, std::uint16_t status_code) {
    CacheEntry e{};
    std::strncpy(e.key, key, kCacheKeyMaxLen - 1);
    e.status_code = status_code;
    return e;
}

TEST(L1CacheTest, UnknownKeyMisses) {
    auto cache = std::make_unique<L1Cache>();
    l1_init(cache.get());
    CacheEntry out{};
    EXPECT_FALSE(l1_get(cache.get(), "missing", 0, &out));
}

TEST(L1CacheTest, PutThenGetHits) {
    auto cache = std::make_unique<L1Cache>();
    l1_init(cache.get());
    l1_put(cache.get(), make_entry("k1", 200));

    CacheEntry out{};
    EXPECT_TRUE(l1_get(cache.get(), "k1", 0, &out));
    EXPECT_EQ(out.status_code, 200u);
}

TEST(L1CacheTest, CapacityOverflowEvictsOldest) {
    auto cache = std::make_unique<L1Cache>();
    l1_init(cache.get());

    l1_put(cache.get(), make_entry("k0", 200));
    CacheEntry out{};
    EXPECT_TRUE(l1_get(cache.get(), "k0", 0, &out));

    // Keep putting new keys until k0 is evicted from its shard
    for (std::size_t i = 1; i < 100000; ++i) {
        std::string key = "k" + std::to_string(i);
        l1_put(cache.get(), make_entry(key.c_str(), 200));
        if (!l1_get(cache.get(), "k0", 0, &out)) {
            break;
        }
    }

    EXPECT_FALSE(l1_get(cache.get(), "k0", 0, &out));
}

TEST(L1CacheTest, StaleSlotMissAfterEviction) {
    auto cache = std::make_unique<L1Cache>();
    l1_init(cache.get());

    l1_put(cache.get(), make_entry("target", 200));

    // Overflow until "target" is evicted from its shard
    for (std::size_t i = 0; i < 100000; ++i) {
        std::string key = "fill" + std::to_string(i);
        l1_put(cache.get(), make_entry(key.c_str(), 300));
        CacheEntry out{};
        if (!l1_get(cache.get(), "target", 0, &out)) {
            break;
        }
    }

    CacheEntry out{};
    EXPECT_FALSE(l1_get(cache.get(), "target", 0, &out));

    // Re-put "target" with new status
    l1_put(cache.get(), make_entry("target", 201));
    EXPECT_TRUE(l1_get(cache.get(), "target", 0, &out));
    EXPECT_EQ(out.status_code, 201u);
}

TEST(L1CacheTest, ExpiredEntryMisses) {
    auto cache = std::make_unique<L1Cache>();
    l1_init(cache.get());

    CacheEntry e = make_entry("expired_key", 200);
    e.expires_at_epoch_ms = 1000;
    l1_put(cache.get(), e);

    CacheEntry out{};
    // now_ms = 2000 > 1000 => expired
    EXPECT_FALSE(l1_get(cache.get(), "expired_key", 2000, &out));
    // now_ms = 500 < 1000 => not expired
    EXPECT_TRUE(l1_get(cache.get(), "expired_key", 500, &out));
    EXPECT_EQ(out.status_code, 200u);
}

} // namespace bytetaper::cache
