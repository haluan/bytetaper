// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/oversized_response_guard.h"

namespace bytetaper::pagination {

OversizedResponseResult check_oversized_response(std::uint32_t threshold, std::size_t body_len) {
    if (threshold == 0 || body_len <= static_cast<std::size_t>(threshold)) {
        return { false, nullptr };
    }
    return { true, "response_still_oversized" };
}

} // namespace bytetaper::pagination
