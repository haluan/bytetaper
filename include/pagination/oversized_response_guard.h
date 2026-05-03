// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstddef>
#include <cstdint>

namespace bytetaper::pagination {

struct OversizedResponseResult {
    bool warned = false;
    const char* reason = nullptr; // "response_still_oversized" or nullptr
};

// Returns warned=true when body_len strictly exceeds threshold.
// threshold=0 disables the check (returns warned=false always).
OversizedResponseResult check_oversized_response(std::uint32_t threshold, std::size_t body_len);

} // namespace bytetaper::pagination
