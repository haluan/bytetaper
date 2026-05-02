// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_CACHE_L1_RING_CACHE_H
#define BYTETAPER_CACHE_L1_RING_CACHE_H

#include "cache/cache_entry.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::cache {

static constexpr std::size_t kL1SlotCount = 8;

struct L1RingCache {
    CacheEntry slots[kL1SlotCount];
    std::uint32_t generations[kL1SlotCount]; // incremented on each write to a slot
    std::size_t write_cursor;                // next write position (mod kL1SlotCount)
};

void l1_init(L1RingCache* cache);

// Stores a copy of entry at slots[write_cursor % kL1SlotCount],
// increments that slot's generation, advances write_cursor.
void l1_put(L1RingCache* cache, const CacheEntry& entry);

// Linear scan: finds first slot whose key matches and is not expired at now_ms.
// Copies the matching entry into *out and returns true. Returns false on miss.
// now_ms == 0 skips expiry check (useful in tests with no real clock).
bool l1_get(const L1RingCache* cache, const char* key, std::int64_t now_ms, CacheEntry* out);

} // namespace bytetaper::cache

#endif // BYTETAPER_CACHE_L1_RING_CACHE_H
