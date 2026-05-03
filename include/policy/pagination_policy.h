// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstdint>

namespace bytetaper::policy {

enum class PaginationMode : std::uint8_t {
    None = 0,
    LimitOffset, // ?limit=N&offset=M query params
};

struct PaginationPolicy {
    bool enabled = false;
    PaginationMode mode = PaginationMode::None;
    char limit_param[32] = "limit";
    char offset_param[32] = "offset";
    std::uint32_t default_limit = 0; // required when enabled
    std::uint32_t max_limit = 0;     // 0 = same as default_limit
    bool upstream_supports_pagination = false;
    std::uint32_t max_response_bytes_warning = 0; // 0 = no warning threshold
};

// Returns nullptr on success, or a static error string on invalid configuration.
const char* validate_pagination_policy(const PaginationPolicy& policy);

} // namespace bytetaper::policy
