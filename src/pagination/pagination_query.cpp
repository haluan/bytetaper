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

PaginationQueryResult parse_pagination_query(const apg::RequestQueryView& query_view,
                                             const char* limit_param, const char* offset_param,
                                             policy::PaginationMode mode) {
    PaginationQueryResult result{};

    const std::size_t limit_len = std::strlen(limit_param);
    const std::size_t offset_len = std::strlen(offset_param);

    for (std::uint8_t i = 0; i < query_view.count; ++i) {
        const auto& param = query_view.params[i];
        if (param.value == nullptr) {
            continue; // segments without '=' are ignored for pagination
        }

        if (param.key_len == limit_len && std::memcmp(param.key, limit_param, param.key_len) == 0) {
            std::uint32_t v = 0;
            if (!parse_uint32(param.value, param.value + param.value_len, &v)) {
                result.parse_error = true;
                result.error_param = "limit";
                return result;
            }
            result.has_limit = true;
            result.limit = v;
        } else if (mode == policy::PaginationMode::LimitOffset && param.key_len == offset_len &&
                   std::memcmp(param.key, offset_param, param.key_len) == 0) {
            std::uint32_t v = 0;
            if (!parse_uint32(param.value, param.value + param.value_len, &v)) {
                result.parse_error = true;
                result.error_param = "offset";
                return result;
            }
            result.has_offset = true;
            result.offset = v;
        }
    }

    return result;
}

PaginationQueryResult parse_pagination_query(const char* raw_query, std::size_t raw_query_len,
                                             const char* limit_param, const char* offset_param,
                                             policy::PaginationMode mode) {
    apg::RequestQueryView temp{};
    apg::parse_query_view(raw_query, raw_query_len, &temp);
    return parse_pagination_query(temp, limit_param, offset_param, mode);
}

} // namespace bytetaper::pagination
