// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/coalescing_policy_validator.h"

#include "policy/cache_policy.h"

namespace bytetaper::policy {

const char* validate_coalescing_policy_safe(const CoalescingPolicy& policy,
                                            const CachePolicy* cache_policy) {
    // 1. Delegate structural checks first
    const char* base_err = validate_coalescing_policy(policy);
    if (base_err != nullptr) {
        return base_err;
    }

    if (!policy.enabled) {
        return nullptr;
    }

    // 2. Excessive wait check
    if (policy.wait_window_ms > 100) {
        return "coalescing.wait_window_ms exceeds safe production limit (100ms)";
    }

    // 3. Unsupported mode check (future-proofing)
    if (policy.mode != CoalescingMode::CacheAssisted) {
        return "unsupported coalescing mode";
    }

    // 4. Missing cache dependency check
    if (policy.require_cache_enabled) {
        if (cache_policy == nullptr || !cache_policy->enabled) {
            return "coalescing requires cache to be enabled";
        }
    }

    // 5. Auth without scope check
    if (policy.allow_authenticated) {
        if (cache_policy == nullptr || cache_policy->auth_scope_header[0] == '\0') {
            return "coalescing.allow_authenticated=true requires cache.auth_scope_header to be set";
        }
    }

    return nullptr;
}

} // namespace bytetaper::policy
