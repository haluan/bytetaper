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

    if (!decision.should_apply_default_limit) {
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
