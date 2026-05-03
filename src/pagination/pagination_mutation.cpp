// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_mutation.h"

#include <cstdio>
#include <cstring>

namespace bytetaper::pagination {

PaginationMutationResult apply_pagination_mutation(const char* raw_path, std::size_t raw_path_len,
                                                   const char* raw_query, std::size_t raw_query_len,
                                                   const PaginationDecision& decision,
                                                   const char* limit_param, char* out_buf,
                                                   std::size_t out_buf_size) {

    if (out_buf == nullptr || out_buf_size == 0)
        return {};

    if (!decision.should_apply_default_limit && !decision.should_cap_limit) {
        // No mutation: copy path + optional query unchanged
        int n;
        if (raw_query_len > 0) {
            n = std::snprintf(out_buf, out_buf_size, "%.*s?%.*s", (int) raw_path_len, raw_path,
                              (int) raw_query_len, raw_query);
        } else {
            n = std::snprintf(out_buf, out_buf_size, "%.*s", (int) raw_path_len, raw_path);
        }
        if (n < 0 || (std::size_t) n >= out_buf_size)
            return {};
        return { false, (std::size_t) n };
    }

    if (decision.should_cap_limit) {
        // Replace the existing limit value in the query with limit_to_apply.
        // Scan for "{limit_param}=" in raw_query and write surrounding parts intact.
        const std::size_t param_len = std::strlen(limit_param);
        const char* q = raw_query;
        const char* q_end = raw_query + raw_query_len;
        const char* match = nullptr;
        const char* val_end = nullptr;

        // Find the segment containing limit_param
        const char* seg = q;
        while (seg < q_end) {
            const char* amp = static_cast<const char*>(
                std::memchr(seg, '&', static_cast<std::size_t>(q_end - seg)));
            const char* seg_end = amp ? amp : q_end;
            const char* eq = static_cast<const char*>(
                std::memchr(seg, '=', static_cast<std::size_t>(seg_end - seg)));
            if (eq && (std::size_t) (eq - seg) == param_len &&
                std::memcmp(seg, limit_param, param_len) == 0) {
                match = seg;
                val_end = seg_end;
                break;
            }
            seg = amp ? amp + 1 : q_end;
        }

        if (match == nullptr) {
            // param not found — fall through to append (shouldn't happen if decision is correct)
            return {};
        }

        // Build: path?{query_before_match}{limit_param}={new_val}{query_after_val}
        int n = std::snprintf(
            out_buf, out_buf_size, "%.*s?%.*s%s=%u%s", (int) raw_path_len, raw_path,
            (int) (match - raw_query), raw_query, limit_param, decision.limit_to_apply,
            val_end < q_end ? val_end : ""); // remainder after the replaced segment
        if (n < 0 || (std::size_t) n >= out_buf_size)
            return {};
        return { true, (std::size_t) n };
    }

    int n;
    if (raw_query_len > 0) {
        n = std::snprintf(out_buf, out_buf_size, "%.*s?%.*s&%s=%u", (int) raw_path_len, raw_path,
                          (int) raw_query_len, raw_query, limit_param, decision.limit_to_apply);
    } else {
        n = std::snprintf(out_buf, out_buf_size, "%.*s?%s=%u", (int) raw_path_len, raw_path,
                          limit_param, decision.limit_to_apply);
    }
    if (n < 0 || (std::size_t) n >= out_buf_size)
        return {};
    return { true, (std::size_t) n };
}

} // namespace bytetaper::pagination
