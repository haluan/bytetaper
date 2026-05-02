// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/cache_policy.h"

namespace bytetaper::policy {

const char* validate_cache_policy(const CachePolicy& policy) {
    if (!policy.enabled) {
        return nullptr; // disabled is always valid
    }
    if (policy.ttl_seconds == 0) {
        return "cache.ttl_seconds required when cache is enabled";
    }
    if (!policy.l1.enabled && !policy.l2.enabled) {
        return "at least one cache layer (l1 or l2) must be enabled";
    }
    if (policy.l2.enabled && policy.l2.path[0] == '\0') {
        return "cache.layers.l2.path required when L2 cache is enabled";
    }
    return nullptr;
}

} // namespace bytetaper::policy
