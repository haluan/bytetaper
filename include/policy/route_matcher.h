// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_ROUTE_MATCHER_H
#define BYTETAPER_POLICY_ROUTE_MATCHER_H

#include "policy/route_policy.h"

#include <cstddef>
#include <cstring>

namespace bytetaper::policy {

static constexpr std::size_t kMaxCompiledRoutes = 64;

struct CompiledExactRoute {
    const char* path = nullptr;
    const RoutePolicy* policy = nullptr;
    std::uint32_t original_order = 0;
};

struct CompiledPrefixRoute {
    const char* prefix = nullptr;
    std::size_t prefix_len = 0;
    const RoutePolicy* policy = nullptr;
    std::uint32_t original_order = 0;
};

struct CompiledRouteMatcher {
    CompiledExactRoute exact_routes[kMaxCompiledRoutes];
    std::size_t exact_count = 0;

    CompiledPrefixRoute prefix_routes[kMaxCompiledRoutes];
    std::size_t prefix_count = 0;
};

inline CompiledRouteMatcher* compile_route_matcher(const RoutePolicy* policies, std::size_t count,
                                                   CompiledRouteMatcher* matcher) {
    matcher->exact_count = 0;
    matcher->prefix_count = 0;

    for (std::size_t i = 0; i < count; ++i) {
        const RoutePolicy& p = policies[i];
        if (p.match_prefix == nullptr) {
            continue;
        }

        if (p.match_kind == RouteMatchKind::Exact) {
            if (matcher->exact_count < kMaxCompiledRoutes) {
                auto& e = matcher->exact_routes[matcher->exact_count++];
                e.path = p.match_prefix;
                e.policy = &p;
                e.original_order = static_cast<std::uint32_t>(i);
            }
        } else {
            if (matcher->prefix_count < kMaxCompiledRoutes) {
                auto& pr = matcher->prefix_routes[matcher->prefix_count++];
                pr.prefix = p.match_prefix;
                pr.prefix_len = std::strlen(p.match_prefix);
                pr.policy = &p;
                pr.original_order = static_cast<std::uint32_t>(i);
            }
        }
    }
    return matcher;
}

inline const RoutePolicy* match_route_compiled(const CompiledRouteMatcher& matcher,
                                               const char* request_path) {
    if (request_path == nullptr) {
        return nullptr;
    }

    const CompiledExactRoute* best_exact = nullptr;
    const CompiledPrefixRoute* best_prefix = nullptr;

    // Scan exact routes
    for (std::size_t i = 0; i < matcher.exact_count; ++i) {
        const auto& e = matcher.exact_routes[i];
        if (std::strcmp(request_path, e.path) == 0) {
            best_exact = &e;
            break; // First match wins in the sorted order
        }
    }

    // Scan prefix routes
    for (std::size_t i = 0; i < matcher.prefix_count; ++i) {
        const auto& pr = matcher.prefix_routes[i];
        if (std::strncmp(request_path, pr.prefix, pr.prefix_len) == 0) {
            best_prefix = &pr;
            break; // First match wins in the sorted order
        }
    }

    if (best_exact == nullptr && best_prefix == nullptr) {
        return nullptr;
    }
    if (best_exact == nullptr) {
        return best_prefix->policy;
    }
    if (best_prefix == nullptr) {
        return best_exact->policy;
    }

    // Both matched — return whichever appeared first in the original array
    return (best_exact->original_order < best_prefix->original_order) ? best_exact->policy
                                                                      : best_prefix->policy;
}

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
