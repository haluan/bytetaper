// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/l1_cache.h"

#include "cache/cache_entry.h"

#include <cstring>

namespace bytetaper::cache {

namespace {

// Threshold for switching between sequential ring buffer and hash indexing.
static constexpr std::size_t kHashLookupThreshold = 256;

// Standard initial value for DJB2 hashing algorithm.
static constexpr std::size_t kDJB2InitialHash = 5381;

// Simple, fast hash function (DJB2) used when cache is large.
std::size_t hash_key(const char* key) {
    if (key == nullptr) {
        return 0;
    }
    std::size_t h = kDJB2InitialHash;
    int c;
    while ((c = static_cast<unsigned char>(*key++))) {
        h = ((h << 5) + h) + static_cast<std::size_t>(c); // h * 33 + c
    }
    return h;
}

} // namespace

void l1_init(L1Cache* cache) {
    if (cache == nullptr) {
        return;
    }
    std::memset(cache, 0, sizeof(L1Cache));
}

void l1_put(L1Cache* cache, const CacheEntry& entry) {
    if (cache == nullptr) {
        return;
    }

    if constexpr (kL1SlotCount > kHashLookupThreshold) {
        // Large cache: Use Hash Index for O(1) performance.
        const std::size_t h = hash_key(entry.key);
        const std::size_t slot_idx = h % kL1SlotCount;
        cache->slots[slot_idx] = entry;
        cache->generations[slot_idx] += 1;
    } else {
        // Small cache: Use Ring Buffer flow (Sequential FIFO).
        const std::size_t slot_idx = cache->write_cursor % kL1SlotCount;
        cache->slots[slot_idx] = entry;
        cache->generations[slot_idx] += 1;
        cache->write_cursor += 1;
    }
}

bool l1_get(const L1Cache* cache, const char* key, std::int64_t now_ms, CacheEntry* out) {
    if (cache == nullptr || key == nullptr || out == nullptr) {
        return false;
    }

    if constexpr (kL1SlotCount > kHashLookupThreshold) {
        // Large cache: Use Hash Index for O(1) lookup.
        const std::size_t h = hash_key(key);
        const std::size_t slot_idx = h % kL1SlotCount;

        if (cache->generations[slot_idx] == 0) {
            return false;
        }
        if (std::strncmp(cache->slots[slot_idx].key, key, kCacheKeyMaxLen) != 0) {
            return false;
        }
        if (now_ms > 0 && is_expired(cache->slots[slot_idx], now_ms)) {
            return false;
        }

        *out = cache->slots[slot_idx];
        return true;
    } else {
        // Small cache: Use Ring Buffer (Linear Scan).
        for (std::size_t i = 0; i < kL1SlotCount; ++i) {
            if (cache->generations[i] == 0) {
                continue;
            }
            if (std::strncmp(cache->slots[i].key, key, kCacheKeyMaxLen) != 0) {
                continue;
            }
            if (now_ms > 0 && is_expired(cache->slots[i], now_ms)) {
                continue;
            }

            *out = cache->slots[i];
            return true;
        }
        return false;
    }
}

} // namespace bytetaper::cache
