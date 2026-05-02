// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/cache_policy_validator.h"

namespace bytetaper::policy {

const char* validate_cache_policy_safe(const CachePolicy& policy, HttpMethod route_method) {
    // Delegate structural checks first
    const char* base_err = validate_cache_policy(policy);
    if (base_err != nullptr) {
        return base_err;
    }

    if (!policy.enabled) {
        return nullptr; // disabled policy needs no further checks
    }

    if (route_method != HttpMethod::Get && route_method != HttpMethod::Any) {
        return "cache not supported for non-GET routes";
    }

    if (policy.private_cache && policy.auth_scope_header[0] == '\0') {
        return "cache.private_cache requires auth_scope_header to be set";
    }

    return nullptr;
}

} // namespace bytetaper::policy
