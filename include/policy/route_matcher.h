// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_ROUTE_MATCHER_H
#define BYTETAPER_POLICY_ROUTE_MATCHER_H

#include <cstddef>
#include <cstring>

#include "policy/route_policy.h"

namespace bytetaper::policy {

/**
 * Finds the first RoutePolicy in [policies, policies+count) whose match_prefix
 * matches `request_path` according to the policy's match_kind.
 *
 * Returns a pointer to the matching RoutePolicy (into the caller's array),
 * or nullptr if no policy matches or if request_path is null.
 *
 * First-match-wins order is guaranteed.
 */
inline const RoutePolicy* match_route_by_path(const RoutePolicy* policies, std::size_t count,
                                              const char* request_path) {
    if (policies == nullptr || request_path == nullptr) {
        return nullptr;
    }

    for (std::size_t i = 0; i < count; ++i) {
        const RoutePolicy& policy = policies[i];

        if (policy.match_prefix == nullptr) {
            continue;
        }

        if (policy.match_kind == RouteMatchKind::Exact) {
            if (std::strcmp(request_path, policy.match_prefix) == 0) {
                return &policy;
            }
        } else {
            // Default: Prefix
            const std::size_t prefix_len = std::strlen(policy.match_prefix);
            if (std::strncmp(request_path, policy.match_prefix, prefix_len) == 0) {
                return &policy;
            }
        }
    }

    return nullptr;
}

} // namespace bytetaper::policy

#endif // BYTETAPER_POLICY_ROUTE_MATCHER_H
