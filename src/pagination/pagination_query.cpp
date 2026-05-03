// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_query.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace bytetaper::pagination {

static bool parse_uint32(const char* value_start, const char* value_end, std::uint32_t* out) {
    if (value_start == value_end)
        return false;
    // reject leading '-'
    if (*value_start == '-')
        return false;
    char* end_ptr = nullptr;
    errno = 0;
    // We need a null-terminated string for strtoul, or we use a temporary buffer.
    // Given the zero-allocation constraint, we'll check characters manually or
    // use a small stack buffer if we must.
    // Actually, strtoul stops at non-digits, but we need to know where it stopped.
    // Since we have val_end, we can temporarily null-terminate if the buffer is mutable,
    // but it's const.
    // Better: use a small stack buffer since limits/offsets are small.
    char buf[16];
    std::size_t len = static_cast<std::size_t>(value_end - value_start);
    if (len >= sizeof(buf))
        return false;
    std::memcpy(buf, value_start, len);
    buf[len] = '\0';

    unsigned long v = std::strtoul(buf, &end_ptr, 10);
    if (end_ptr != (buf + len))
        return false; // trailing non-numeric chars
    if (errno != 0)
        return false; // overflow
    if (v > std::uint32_t{ 0xFFFFFFFF })
        return false;
    *out = static_cast<std::uint32_t>(v);
    return true;
}

PaginationQueryResult parse_pagination_query(const char* raw_query, std::size_t raw_query_len,
                                             const char* limit_param, const char* offset_param) {

    PaginationQueryResult result{};
    if (raw_query == nullptr || raw_query_len == 0) {
        return result;
    }

    const std::size_t limit_len = std::strlen(limit_param);
    const std::size_t offset_len = std::strlen(offset_param);

    const char* seg = raw_query;
    const char* end = raw_query + raw_query_len;

    while (seg < end) {
        const char* amp =
            static_cast<const char*>(std::memchr(seg, '&', static_cast<std::size_t>(end - seg)));
        const char* seg_end = (amp != nullptr) ? amp : end;

        const char* eq = static_cast<const char*>(
            std::memchr(seg, '=', static_cast<std::size_t>(seg_end - seg)));

        if (eq != nullptr) {
            const std::size_t key_len = static_cast<std::size_t>(eq - seg);
            const char* val_start = eq + 1;
            const char* val_end = seg_end;

            if (key_len == limit_len && std::memcmp(seg, limit_param, key_len) == 0) {
                std::uint32_t v = 0;
                if (!parse_uint32(val_start, val_end, &v)) {
                    result.parse_error = true;
                    result.error_param = "limit";
                    return result;
                }
                result.has_limit = true;
                result.limit = v;
            } else if (key_len == offset_len && std::memcmp(seg, offset_param, key_len) == 0) {
                std::uint32_t v = 0;
                if (!parse_uint32(val_start, val_end, &v)) {
                    result.parse_error = true;
                    result.error_param = "offset";
                    return result;
                }
                result.has_offset = true;
                result.offset = v;
            }
        }

        seg = (amp != nullptr) ? amp + 1 : end;
    }

    return result;
}

} // namespace bytetaper::pagination
