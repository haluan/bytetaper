// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/l1_cache.h"

#include "cache/cache_entry.h"

#include <cstring>

namespace bytetaper::cache {

namespace {

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

void copy_body_to_slot(L1Cache* cache, std::size_t slot_idx, const CacheEntry& entry) {
    if (entry.body == nullptr || entry.body_len == 0) {
        cache->slots[slot_idx].body = nullptr;
        cache->slots[slot_idx].body_len = 0;
        return;
    }

    std::size_t copy_len = (entry.body_len > kL1MaxBodySize) ? kL1MaxBodySize : entry.body_len;
    std::memcpy(cache->bodies[slot_idx], entry.body, copy_len);
    cache->slots[slot_idx].body = cache->bodies[slot_idx];
    cache->slots[slot_idx].body_len = copy_len;
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

    if constexpr (kL1SlotCount >= kHashLookupThreshold) {
        // Large cache: Use Hash Index for O(1) performance.
        const std::size_t h = hash_key(entry.key);
        const std::size_t slot_idx = h % kL1SlotCount;
        cache->slots[slot_idx] = entry;
        copy_body_to_slot(cache, slot_idx, entry);
        cache->generations[slot_idx] += 1;
    } else {
        // Small cache: Use Ring Buffer flow (Sequential FIFO).
        const std::size_t slot_idx = cache->write_cursor % kL1SlotCount;
        cache->slots[slot_idx] = entry;
        copy_body_to_slot(cache, slot_idx, entry);
        cache->generations[slot_idx] += 1;
        cache->write_cursor += 1;
    }
}

bool l1_get(const L1Cache* cache, const char* key, std::int64_t now_ms, CacheEntry* out) {
    if (cache == nullptr || key == nullptr || out == nullptr) {
        return false;
    }

    if constexpr (kL1SlotCount >= kHashLookupThreshold) {
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
        // Ensure out->body points to the slot's buffer, not the original source
        if (out->body_len > 0) {
            out->body = cache->bodies[slot_idx];
        }
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
            // Ensure out->body points to the slot's buffer, not the original source
            if (out->body_len > 0) {
                out->body = cache->bodies[i];
            }
            return true;
        }
        return false;
    }
}

} // namespace bytetaper::cache
