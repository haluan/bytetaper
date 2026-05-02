// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

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

    try {
        rocksdb::Options options;
        options.create_if_missing = true;
        std::unique_ptr<rocksdb::DB> db;
        rocksdb::Status status = rocksdb::DB::Open(options, path, &db);
        if (!status.ok()) {
            return nullptr;
        }
        return new L2DiskCache{ db.release() };
    } catch (...) {
        return nullptr;
    }
}

void l2_close(L2DiskCache** cache) {
    if (!cache || !*cache)
        return;

    try {
        delete (*cache)->db;
        delete *cache;
        *cache = nullptr;
    } catch (...) {
        // Silently fail to ensure no exception propagation
    }
}

bool l2_destroy(const char* path) {
    if (!path)
        return false;

    try {
        rocksdb::Status s = rocksdb::DestroyDB(path, rocksdb::Options{});
        return s.ok();
    } catch (...) {
        return false;
    }
}

bool l2_put(L2DiskCache* cache, const CacheEntry& entry) {
    if (!cache || !cache->db)
        return false;

    try {
        // Serialization format:
        // [uint16_t  status_code         ]   2 bytes
        // [uint64_t  body_len            ]   8 bytes
        // [char[64]  content_type        ]  64 bytes
        // [int64_t   created_at_epoch_ms ]   8 bytes
        // [int64_t   expires_at_epoch_ms ]   8 bytes
        // [char*     body data           ]  body_len bytes

        std::string value;
        std::size_t header_size = 2 + 8 + kCacheContentTypeMaxLen + 8 + 8;
        value.resize(header_size + entry.body_len);

        char* p = &value[0];

        std::uint16_t sc = entry.status_code;
        std::memcpy(p, &sc, 2);
        p += 2;

        std::uint64_t bl = static_cast<std::uint64_t>(entry.body_len);
        std::memcpy(p, &bl, 8);
        p += 8;

        std::memcpy(p, entry.content_type, kCacheContentTypeMaxLen);
        p += kCacheContentTypeMaxLen;

        std::int64_t ca = entry.created_at_epoch_ms;
        std::memcpy(p, &ca, 8);
        p += 8;

        std::int64_t ea = entry.expires_at_epoch_ms;
        std::memcpy(p, &ea, 8);
        p += 8;

        if (entry.body_len > 0 && entry.body) {
            std::memcpy(p, entry.body, entry.body_len);
        }

        rocksdb::Status s = cache->db->Put(rocksdb::WriteOptions(), entry.key, value);
        return s.ok();
    } catch (...) {
        return false;
    }
}

bool l2_get(L2DiskCache* cache, const char* key, std::int64_t now_ms, CacheEntry* out,
            char* body_buf, std::size_t body_buf_size) {
    if (!cache || !cache->db || !key || !out)
        return false;

    try {
        std::string raw;
        rocksdb::Status s = cache->db->Get(rocksdb::ReadOptions(), key, &raw);
        if (!s.ok()) {
            return false;
        }

        std::size_t header_size = 2 + 8 + kCacheContentTypeMaxLen + 8 + 8;
        if (raw.size() < header_size) {
            return false;
        }

        const char* p = raw.data();

        std::uint16_t sc;
        std::memcpy(&sc, p, 2);
        p += 2;
        out->status_code = sc;

        std::uint64_t bl;
        std::memcpy(&bl, p, 8);
        p += 8;
        out->body_len = static_cast<std::size_t>(bl);

        std::memcpy(out->content_type, p, kCacheContentTypeMaxLen);
        p += kCacheContentTypeMaxLen;

        std::int64_t ca;
        std::memcpy(&ca, p, 8);
        p += 8;
        out->created_at_epoch_ms = ca;

        std::int64_t ea;
        std::memcpy(&ea, p, 8);
        p += 8;
        out->expires_at_epoch_ms = ea;

        // TTL Check
        if (now_ms > 0 && !cache_ttl_valid(now_ms, out->expires_at_epoch_ms)) {
            return false;
        }

        if (out->body_len > body_buf_size) {
            return false;
        }

        if (out->body_len > 0) {
            std::memcpy(body_buf, p, out->body_len);
            out->body = body_buf;
        } else {
            out->body = nullptr;
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool l2_remove(L2DiskCache* cache, const char* key) {
    if (!cache || !cache->db || !key)
        return false;

    try {
        rocksdb::Status s = cache->db->Delete(rocksdb::WriteOptions(), key);
        return s.ok();
    } catch (...) {
        return false;
    }
}

} // namespace bytetaper::cache
