// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry_codec.h"
#include "cache/cache_ttl.h"
#include "cache/l2_disk_cache.h"

#include <cstring>
#include <memory>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <string>
#include <vector>

namespace bytetaper::cache {

struct L2DiskCache {
    rocksdb::DB* db = nullptr;
};

L2DiskCache* l2_open(const char* path) {
    if (!path)
        return nullptr;

    rocksdb::Options options;
    options.create_if_missing = true;
    std::unique_ptr<rocksdb::DB> db;
    rocksdb::Status status = rocksdb::DB::Open(options, path, &db);
    if (!status.ok()) {
        return nullptr;
    }
    return new L2DiskCache{ db.release() };
}

void l2_close(L2DiskCache** cache) {
    if (!cache || !*cache)
        return;

    delete (*cache)->db;
    delete *cache;
    *cache = nullptr;
}

bool l2_destroy(const char* path) {
    if (!path)
        return false;

    rocksdb::Status s = rocksdb::DestroyDB(path, rocksdb::Options{});
    return s.ok();
}

bool l2_put(L2DiskCache* cache, const CacheEntry& entry) {
    if (!cache || !cache->db)
        return false;

    const std::size_t enc_size = kCacheEntryEncodedOverhead + entry.body_len;
    std::vector<char> enc_buf(enc_size);
    const std::size_t written = cache_entry_encode(entry, enc_buf.data(), enc_buf.size());
    if (written == 0) {
        return false;
    }

    rocksdb::Status s =
        cache->db->Put(rocksdb::WriteOptions(), entry.key, rocksdb::Slice(enc_buf.data(), written));
    return s.ok();
}

bool l2_get(L2DiskCache* cache, const char* key, std::int64_t now_ms, CacheEntry* out,
            char* body_buf, std::size_t body_buf_size) {
    if (!cache || !cache->db || !key || !out)
        return false;

    std::string raw;
    rocksdb::Status s = cache->db->Get(rocksdb::ReadOptions(), key, &raw);
    if (!s.ok()) {
        return false;
    }

    CacheEntry decoded{};
    bool ok = cache_entry_decode(raw.data(), raw.size(), &decoded, body_buf, body_buf_size);
    if (!ok) {
        return false;
    }

    // TTL Check
    if (now_ms > 0 && !cache_ttl_valid(now_ms, decoded.expires_at_epoch_ms)) {
        return false;
    }

    *out = decoded;
    return true;
}

bool l2_remove(L2DiskCache* cache, const char* key) {
    if (!cache || !cache->db || !key)
        return false;

    rocksdb::Status s = cache->db->Delete(rocksdb::WriteOptions(), key);
    return s.ok();
}

} // namespace bytetaper::cache
