// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/compression_policy.h"

namespace bytetaper::policy {

const char* validate_compression_policy_safe(const CompressionPolicy& policy) {
    // 1. Delegate to base structural checks
    const char* base_err = validate_compression_policy(policy);
    if (base_err != nullptr) {
        return base_err;
    }

    // 2. Disabled policy is always safe
    if (!policy.enabled) {
        return nullptr;
    }

    // 3. Reject min_size_bytes <= 0 when enabled
    if (policy.min_size_bytes == 0) {
        return "compression enabled with min_size_bytes <= 0";
    }

    // 4. Reject empty content-type allowlist
    if (policy.eligible_content_type_count == 0) {
        return "compression enabled with empty eligible_content_types";
    }

    // 5. Reject missing or invalid preferred algorithms
    if (policy.preferred_algorithm_count == 0) {
        return "compression enabled with no preferred_algorithms";
    }

    for (std::size_t i = 0; i < policy.preferred_algorithm_count; ++i) {
        if (policy.preferred_algorithms[i] == CompressionAlgorithm::None) {
            return "compression.preferred_algorithms contains 'none'";
        }
    }

    // 6. Reject already_encoded_behavior != skip
    if (policy.already_encoded_behavior != AlreadyEncodedBehavior::Skip) {
        return "compression.already_encoded_behavior must be 'skip'";
    }

    return nullptr;
}

} // namespace bytetaper::policy
