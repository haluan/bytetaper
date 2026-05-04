// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_key.h"

#include <cstdio>

namespace bytetaper::coalescing {

bool build_coalescing_key(const CoalescingKeyInput& input, char* out_buf, std::size_t out_size) {
    if (out_buf == nullptr || out_size == 0) {
        return false;
    }

    // Only GET requests are eligible for coalescing per spec.
    if (input.method != policy::HttpMethod::Get) {
        return false;
    }

    // Required fields per spec.
    if (input.route_name == nullptr || input.normalized_path == nullptr) {
        return false;
    }

    const char* query = (input.normalized_query != nullptr) ? input.normalized_query : "";
    const char* fields = (input.selected_fields != nullptr) ? input.selected_fields : "";

    // Format: c_key:{route}:{version}:{path}?{query}&fields={fields}
    // We use snprintf for safe, bounded string construction.
    int written = std::snprintf(out_buf, out_size, "c_key:%s:%u:%s?%s&fields=%s", input.route_name,
                                input.policy_version, input.normalized_path, query, fields);

    // snprintf returns the number of characters that WOULD have been written (excluding null).
    // If written >= out_size, it means truncation occurred.
    if (written < 0 || static_cast<std::size_t>(written) >= out_size) {
        return false;
    }

    return true;
}

} // namespace bytetaper::coalescing
