// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_COALESCING_COALESCING_KEY_H
#define BYTETAPER_COALESCING_COALESCING_KEY_H

#include "policy/method_matcher.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::coalescing {

/**
 * @brief Input for deterministic coalescing key generation.
 * Following Orthodox C++ style, using plain pointers for string views
 * and fixed layout to ensure zero-allocation processing.
 */
struct CoalescingKeyInput {
    policy::HttpMethod method = policy::HttpMethod::Get;
    const char* route_name = nullptr;
    const char* normalized_path = nullptr;
    const char* normalized_query = nullptr;
    const char* selected_fields = nullptr;
    std::uint32_t policy_version = 0;
};

/**
 * @brief Writes a deterministic null-terminated coalescing key into out_buf.
 *
 * The key is used to group identical in-flight GET requests.
 * Format: c_key:{route}:{version}:{path}?{query}&fields={fields}
 *
 * @param input The request properties to include in the key.
 * @param out_buf Buffer to write the key into.
 * @param out_size Size of the output buffer.
 * @return true if the key was successfully written.
 * @return false if:
 *   - method is not Get
 *   - route_name or normalized_path is null
 *   - out_buf is null
 *   - the resulting key would exceed out_size (including null terminator)
 */
bool build_coalescing_key(const CoalescingKeyInput& input, char* out_buf, std::size_t out_size);

} // namespace bytetaper::coalescing

#endif // BYTETAPER_COALESCING_COALESCING_KEY_H
