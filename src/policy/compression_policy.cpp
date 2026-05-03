// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/compression_policy.h"

namespace bytetaper::policy {

const char* validate_compression_policy(const CompressionPolicy& policy) {
    if (!policy.enabled) {
        return nullptr;
    }
    if (policy.eligible_content_type_count > kMaxEligibleContentTypes) {
        return "compression.eligible_content_types exceeds maximum count";
    }
    if (policy.preferred_algorithm_count > kMaxCompressionAlgorithms) {
        return "compression.preferred_algorithms exceeds maximum count";
    }
    return nullptr;
}

} // namespace bytetaper::policy
