// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/l1_cache.h"

#include <cstring>
#include <gtest/gtest.h>
#include <string>

namespace bytetaper::cache {

static CacheEntry make_entry(const char* key, std::uint16_t status_code) {
    CacheEntry e{};
    std::strncpy(e.key, key, kCacheKeyMaxLen - 1);
    e.status_code = status_code;
    return e;
}

TEST(L1CacheTest, UnknownKeyMisses) {
    L1Cache cache;
    l1_init(&cache);
    CacheEntry out{};
    EXPECT_FALSE(l1_get(&cache, "missing", 0, &out));
}

TEST(L1CacheTest, PutThenGetHits) {
    L1Cache cache;
    l1_init(&cache);
    l1_put(&cache, make_entry("k1", 200));

    CacheEntry out{};
    EXPECT_TRUE(l1_get(&cache, "k1", 0, &out));
    EXPECT_EQ(out.status_code, 200u);
}

TEST(L1CacheTest, CapacityOverflowEvictsOldest) {
    L1Cache cache;
    l1_init(&cache);

    // Fill capacity
    for (std::size_t i = 0; i < kL1SlotCount; ++i) {
        std::string key = "k" + std::to_string(i);
        l1_put(&cache, make_entry(key.c_str(), 200 + static_cast<std::uint16_t>(i)));
    }

    // k0 should still be there
    CacheEntry out{};
    EXPECT_TRUE(l1_get(&cache, "k0", 0, &out));

    // One more put should evict k0 (the oldest)
    l1_put(&cache, make_entry("overflow", 299));
    EXPECT_FALSE(l1_get(&cache, "k0", 0, &out));
    EXPECT_TRUE(l1_get(&cache, "overflow", 0, &out));
}

TEST(L1CacheTest, StaleSlotMissAfterEviction) {
    L1Cache cache;
    l1_init(&cache);

    l1_put(&cache, make_entry("target", 200));

    // Overflow ring with kL1SlotCount distinct puts to evict "target"
    for (std::size_t i = 0; i < kL1SlotCount; ++i) {
        std::string key = "fill" + std::to_string(i);
        l1_put(&cache, make_entry(key.c_str(), 300));
    }

    CacheEntry out{};
    EXPECT_FALSE(l1_get(&cache, "target", 0, &out));

    // Re-put "target" with new status
    l1_put(&cache, make_entry("target", 201));
    EXPECT_TRUE(l1_get(&cache, "target", 0, &out));
    EXPECT_EQ(out.status_code, 201u);
}

TEST(L1CacheTest, ExpiredEntryMisses) {
    L1Cache cache;
    l1_init(&cache);

    CacheEntry e = make_entry("expired_key", 200);
    e.expires_at_epoch_ms = 1000;
    l1_put(&cache, e);

    CacheEntry out{};
    // now_ms = 2000 > 1000 => expired
    EXPECT_FALSE(l1_get(&cache, "expired_key", 2000, &out));
    // now_ms = 500 < 1000 => not expired
    EXPECT_TRUE(l1_get(&cache, "expired_key", 500, &out));
    EXPECT_EQ(out.status_code, 200u);
}

} // namespace bytetaper::cache
