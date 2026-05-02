// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_CACHE_L2_DISK_CACHE_H
#define BYTETAPER_CACHE_L2_DISK_CACHE_H

#include "cache/cache_entry.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::cache {

struct L2DiskCache; // opaque; defined in l2_rocksdb_cache.cpp

static constexpr std::size_t kL2MaxBodySize = 1 * 1024 * 1024; // 1 MiB

// Opens (or creates) a RocksDB database at path. Returns nullptr on failure.
L2DiskCache* l2_open(const char* path);

// Closes and frees the database handle. Sets *cache to nullptr.
void l2_close(L2DiskCache** cache);

// Destroys the on-disk database at path (use for test cleanup).
bool l2_destroy(const char* path);

// Stores entry in the database. Returns true on success.
bool l2_put(L2DiskCache* cache, const CacheEntry& entry);

// Retrieves entry by key.
// body_buf[body_buf_size] is caller-owned storage; out->body points into it on hit.
// If now_ms > 0 and the stored entry is expired, returns false.
// Returns false on miss or if stored body exceeds body_buf_size.
bool l2_get(L2DiskCache* cache, const char* key, std::int64_t now_ms, CacheEntry* out,
            char* body_buf, std::size_t body_buf_size);

// Removes the entry for key. Returns true if the key existed.
bool l2_remove(L2DiskCache* cache, const char* key);

} // namespace bytetaper::cache

#endif // BYTETAPER_CACHE_L2_DISK_CACHE_H
