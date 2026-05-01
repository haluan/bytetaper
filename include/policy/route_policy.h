// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_ROUTE_POLICY_H
#define BYTETAPER_POLICY_ROUTE_POLICY_H

#include <cstdint>
#include <cstring>

#include "policy/field_filter_policy.h"

namespace bytetaper::policy {

enum class RouteMatchKind : std::uint8_t {
    Prefix = 0,
    Exact = 1,
};

enum class MutationMode : std::uint8_t {
    Disabled = 0,
    HeadersOnly = 1,
    Full = 2,
};

enum class HttpMethod : std::uint8_t {
    Any = 0,
    Get = 1,
    Post = 2,
    Put = 3,
    Delete = 4,
    Patch = 5,
};

struct RoutePolicy {
    const char* route_id = nullptr;
    const char* match_prefix = nullptr;
    RouteMatchKind match_kind = RouteMatchKind::Prefix;
    MutationMode mutation = MutationMode::Disabled;
    HttpMethod allowed_method = HttpMethod::Any;
    FieldFilterPolicy field_filter = {};
};

// Validates a RoutePolicy. Returns true if usable.
// On failure, *reason_out (if non-null) is set to a static string describing the problem.
inline bool validate_route_policy(const RoutePolicy& policy, const char** reason_out) {
    if (policy.route_id == nullptr || policy.route_id[0] == '\0') {
        if (reason_out != nullptr) {
            *reason_out = "route_id is required";
        }
        return false;
    }
    if (policy.match_kind == RouteMatchKind::Prefix) {
        if (policy.match_prefix == nullptr || policy.match_prefix[0] != '/') {
            if (reason_out != nullptr) {
                *reason_out = "match_prefix must start with '/'";
            }
            return false;
        }
    }
    return true;
}

} // namespace bytetaper::policy

#endif // BYTETAPER_POLICY_ROUTE_POLICY_H
