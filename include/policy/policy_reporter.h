// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_POLICY_REPORTER_H
#define BYTETAPER_POLICY_POLICY_REPORTER_H

#include "policy/cache_policy.h"
#include "policy/field_filter_policy.h"
#include "policy/route_policy.h"
#include <cstdio>

namespace bytetaper::policy {

static inline const char* route_match_kind_str(RouteMatchKind k) {
    switch (k) {
    case RouteMatchKind::Prefix:
        return "prefix";
    case RouteMatchKind::Exact:
        return "exact";
    }
    return "unknown";
}

static inline const char* mutation_mode_str(MutationMode m) {
    switch (m) {
    case MutationMode::Disabled:
        return "disabled";
    case MutationMode::HeadersOnly:
        return "headers_only";
    case MutationMode::Full:
        return "full";
    }
    return "unknown";
}

static inline const char* http_method_str(HttpMethod m) {
    switch (m) {
    case HttpMethod::Any:
        return "any";
    case HttpMethod::Get:
        return "get";
    case HttpMethod::Post:
        return "post";
    case HttpMethod::Put:
        return "put";
    case HttpMethod::Delete:
        return "delete";
    case HttpMethod::Patch:
        return "patch";
    }
    return "unknown";
}

static inline const char* cache_behavior_str(CacheBehavior b) {
    switch (b) {
    case CacheBehavior::Default:
        return "default";
    case CacheBehavior::Bypass:
        return "bypass";
    case CacheBehavior::Store:
        return "store";
    }
    return "unknown";
}

static inline const char* field_filter_mode_str(FieldFilterMode m) {
    switch (m) {
    case FieldFilterMode::None:
        return "none";
    case FieldFilterMode::Allowlist:
        return "allowlist";
    case FieldFilterMode::Denylist:
        return "denylist";
    }
    return "unknown";
}

inline void print_route_policy_report(std::FILE* out, const RoutePolicy* policies,
                                      std::size_t count) {
    if (out == nullptr || policies == nullptr) {
        return;
    }

    std::fprintf(out, "Policy report: %zu route(s)\n", count);

    for (std::size_t i = 0; i < count; ++i) {
        const RoutePolicy& p = policies[i];
        std::fprintf(out, "\n[%zu] id: %s\n", i + 1, p.route_id ? p.route_id : "(null)");
        std::fprintf(out, "    match:  %s %s\n", route_match_kind_str(p.match_kind),
                     p.match_prefix ? p.match_prefix : "(null)");
        std::fprintf(out, "    method: %s\n", http_method_str(p.allowed_method));
        std::fprintf(out, "    mutation: %s\n", mutation_mode_str(p.mutation));

        if (p.max_response_bytes == 0) {
            std::fprintf(out, "    max_response_bytes: unlimited\n");
        } else {
            std::fprintf(out, "    max_response_bytes: %u\n", p.max_response_bytes);
        }

        if (p.cache.behavior == CacheBehavior::Store) {
            std::fprintf(out, "    cache: %s (ttl: %us)\n", cache_behavior_str(p.cache.behavior),
                         p.cache.ttl_seconds);
        } else {
            std::fprintf(out, "    cache: %s\n", cache_behavior_str(p.cache.behavior));
        }

        if (p.field_filter.mode == FieldFilterMode::None) {
            std::fprintf(out, "    field_filter: %s\n", field_filter_mode_str(p.field_filter.mode));
        } else {
            std::fprintf(out, "    field_filter: %s [", field_filter_mode_str(p.field_filter.mode));
            for (std::size_t j = 0; j < p.field_filter.field_count; ++j) {
                std::fprintf(out, "%s%s", p.field_filter.fields[j],
                             (j == p.field_filter.field_count - 1) ? "" : ", ");
            }
            std::fprintf(out, "]\n");
        }
    }
}

} // namespace bytetaper::policy

#endif // BYTETAPER_POLICY_POLICY_REPORTER_H
