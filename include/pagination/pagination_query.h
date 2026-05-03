// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "policy/pagination_policy.h"

#include <cstddef>

namespace bytetaper::pagination {

struct PaginationQueryResult {
    bool has_limit = false;
    std::uint32_t limit = 0;
    bool has_offset = false;
    std::uint32_t offset = 0;
    bool parse_error = false;
    const char* error_param = nullptr; // "limit" or "offset" — static string
};

// Parses limit and offset from a raw query string (e.g., "limit=100&offset=20&fields=id").
// limit_param and offset_param must be non-null; raw_query may be null (treated as empty).
// Offset is only parsed when mode == PaginationMode::LimitOffset.
// Returns parse_error=true if a matched param has a non-numeric or negative value.
// Missing parameters are safe: has_limit/has_offset remain false.
PaginationQueryResult parse_pagination_query(const char* raw_query, std::size_t raw_query_len,
                                             const char* limit_param, const char* offset_param,
                                             policy::PaginationMode mode);

} // namespace bytetaper::pagination
