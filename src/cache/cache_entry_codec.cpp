// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry_codec.h"

#include <cstring>

namespace bytetaper::cache {

std::size_t cache_entry_encode(const CacheEntry& entry, char* buf, std::size_t buf_size) {
    if (buf == nullptr || buf_size < kCacheEntryEncodedOverhead + entry.body_len) {
        return 0;
    }

    char* p = buf;

    // key
    std::memcpy(p, entry.key, kCacheKeyMaxLen);
    p += kCacheKeyMaxLen;

    // status_code
    std::uint16_t sc = entry.status_code;
    std::memcpy(p, &sc, 2);
    p += 2;

    // content_type
    std::memcpy(p, entry.content_type, kCacheContentTypeMaxLen);
    p += kCacheContentTypeMaxLen;

    // body_len
    std::uint64_t bl = static_cast<std::uint64_t>(entry.body_len);
    std::memcpy(p, &bl, 8);
    p += 8;

    // created_at
    std::int64_t ca = entry.created_at_epoch_ms;
    std::memcpy(p, &ca, 8);
    p += 8;

    // expires_at
    std::int64_t ea = entry.expires_at_epoch_ms;
    std::memcpy(p, &ea, 8);
    p += 8;

    // body
    if (entry.body_len > 0 && entry.body != nullptr) {
        std::memcpy(p, entry.body, entry.body_len);
    }

    return kCacheEntryEncodedOverhead + entry.body_len;
}

bool cache_entry_decode(const char* buf, std::size_t buf_size, CacheEntry* out, char* body_buf,
                        std::size_t body_buf_size) {
    if (buf == nullptr || buf_size < kCacheEntryEncodedOverhead || out == nullptr) {
        return false;
    }

    const char* p = buf;

    // key
    std::memcpy(out->key, p, kCacheKeyMaxLen);
    p += kCacheKeyMaxLen;

    // status_code
    std::uint16_t sc;
    std::memcpy(&sc, p, 2);
    out->status_code = sc;
    p += 2;

    // content_type
    std::memcpy(out->content_type, p, kCacheContentTypeMaxLen);
    p += kCacheContentTypeMaxLen;

    // body_len
    std::uint64_t bl;
    std::memcpy(&bl, p, 8);
    out->body_len = static_cast<std::size_t>(bl);
    p += 8;

    // created_at
    std::int64_t ca;
    std::memcpy(&ca, p, 8);
    out->created_at_epoch_ms = ca;
    p += 8;

    // expires_at
    std::int64_t ea;
    std::memcpy(&ea, p, 8);
    out->expires_at_epoch_ms = ea;
    p += 8;

    // body
    if (out->body_len > body_buf_size) {
        return false;
    }

    if (out->body_len > 0) {
        if (body_buf == nullptr)
            return false;
        std::memcpy(body_buf, p, out->body_len);
        out->body = body_buf;
    } else {
        out->body = nullptr;
    }

    return true;
}

} // namespace bytetaper::cache
