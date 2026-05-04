// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_CACHE_L1_CACHE_H
#define BYTETAPER_CACHE_L1_CACHE_H

#include "cache/cache_entry.h"

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace bytetaper::cache {

static constexpr std::size_t kL1ShardCount = 256;
static constexpr std::size_t kL1SlotsPerShard = 16;
static constexpr std::size_t kL1MaxBodySize = 3072; // 3KiB per slot

struct L1CacheShard {
    mutable std::mutex mutex;
    CacheEntry slots[kL1SlotsPerShard];
    char bodies[kL1SlotsPerShard][kL1MaxBodySize];
    std::uint32_t generations[kL1SlotsPerShard];
    std::size_t write_cursor;
};

struct L1Cache {
    L1CacheShard shards[kL1ShardCount];
};

void l1_init(L1Cache* cache);

// Stores a copy of entry in the appropriate shard.
void l1_put(L1Cache* cache, const CacheEntry& entry);

// Promotes entry into L1 only if no newer entry for the same key already exists.
// "Newer" means existing.created_at_epoch_ms > entry.created_at_epoch_ms.
// If the key is not present in L1, promotes unconditionally.
void l1_put_if_newer(L1Cache* cache, const CacheEntry& entry);

// Retrieves an entry by key from the appropriate shard.
// Copies the matching entry into *out and returns true. Returns false on miss.
// now_ms == 0 skips expiry check (useful in tests with no real clock).
bool l1_get(const L1Cache* cache, const char* key, std::int64_t now_ms, CacheEntry* out);

} // namespace bytetaper::cache

#endif // BYTETAPER_CACHE_L1_CACHE_H
