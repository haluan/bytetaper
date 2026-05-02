// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_CACHE_L1_CACHE_H
#define BYTETAPER_CACHE_L1_CACHE_H

#include "cache/cache_entry.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::cache {

static constexpr std::size_t kL1SlotCount = 16;
static constexpr std::size_t kHashLookupThreshold = 64; // Use Ring Buffer for small counts
static constexpr std::size_t kL1MaxBodySize = 3072;     // 3KiB per slot

struct L1Cache {
    CacheEntry slots[kL1SlotCount];
    char bodies[kL1SlotCount][kL1MaxBodySize];
    std::uint32_t generations[kL1SlotCount]; // incremented on each write to a slot
    std::size_t write_cursor;                // next write position (mod kL1SlotCount)
};

void l1_init(L1Cache* cache);

// Stores a copy of entry at slots[write_cursor % kL1SlotCount] or hash index,
// depending on kL1SlotCount threshold.
void l1_put(L1Cache* cache, const CacheEntry& entry);

// Retrieves an entry by key in O(1) or O(N) depending on size.
// Copies the matching entry into *out and returns true. Returns false on miss.
// now_ms == 0 skips expiry check (useful in tests with no real clock).
bool l1_get(const L1Cache* cache, const char* key, std::int64_t now_ms, CacheEntry* out);

} // namespace bytetaper::cache

#endif // BYTETAPER_CACHE_L1_CACHE_H
