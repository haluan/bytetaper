// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/l1_ring_cache.h"

#include "cache/cache_entry.h"

#include <cstring>

namespace bytetaper::cache {

void l1_init(L1RingCache* cache) {
    if (cache == nullptr) {
        return;
    }
    std::memset(cache, 0, sizeof(L1RingCache));
}

void l1_put(L1RingCache* cache, const CacheEntry& entry) {
    if (cache == nullptr) {
        return;
    }
    const std::size_t slot_idx = cache->write_cursor % kL1SlotCount;
    cache->slots[slot_idx] = entry;
    cache->generations[slot_idx] += 1;
    cache->write_cursor += 1;
}

bool l1_get(const L1RingCache* cache, const char* key, std::int64_t now_ms, CacheEntry* out) {
    if (cache == nullptr || key == nullptr || out == nullptr) {
        return false;
    }

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

} // namespace bytetaper::cache
