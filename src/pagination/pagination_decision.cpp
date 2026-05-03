// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_decision.h"

namespace bytetaper::pagination {

PaginationDecision make_pagination_decision(const policy::PaginationPolicy& policy,
                                            const PaginationQueryResult& query) {

    if (!policy.enabled)
        return {};
    if (!policy.upstream_supports_pagination)
        return {};
    if (query.parse_error)
        return {};

    // Cap: client limit exceeds policy max_limit
    if (query.has_limit && policy.max_limit > 0 && query.limit > policy.max_limit) {
        return { false, true, policy.max_limit, "limit_exceeds_max" };
    }

    if (query.has_limit) {
        return { false, false, query.limit, "limit_valid" };
    }

    return { true, false, policy.default_limit, nullptr };
}

} // namespace bytetaper::pagination
