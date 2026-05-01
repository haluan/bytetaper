// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_METHOD_MATCHER_H
#define BYTETAPER_POLICY_METHOD_MATCHER_H

#include <cstring>
#include <strings.h> // for strcasecmp on POSIX systems

#include "policy/route_policy.h"

namespace bytetaper::policy {

/**
 * Checks if the request HTTP method satisfies the policy's allowed_method constraint.
 *
 * Returns true if allowed is HttpMethod::Any or if request_method matches the allowed enum value.
 * Matching is case-insensitive.
 *
 * Returns false if request_method is null or if it does not match a specific constraint.
 */
inline bool match_method(HttpMethod allowed, const char* request_method) {
    if (request_method == nullptr) {
        return false;
    }
    if (allowed == HttpMethod::Any) {
        return true;
    }

    struct MethodEntry {
        HttpMethod method;
        const char* name;
    };

    static constexpr MethodEntry table[] = {
        { HttpMethod::Get, "GET" },     { HttpMethod::Post, "POST" },
        { HttpMethod::Put, "PUT" },     { HttpMethod::Delete, "DELETE" },
        { HttpMethod::Patch, "PATCH" },
    };

    for (const auto& entry : table) {
        if (entry.method == allowed) {
            return strcasecmp(request_method, entry.name) == 0;
        }
    }

    return false;
}

} // namespace bytetaper::policy

#endif // BYTETAPER_POLICY_METHOD_MATCHER_H
