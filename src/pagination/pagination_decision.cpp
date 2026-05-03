// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "pagination/pagination_decision.h"

namespace bytetaper::pagination {

PaginationDecision make_pagination_decision(const policy::PaginationPolicy& policy,
                                            const PaginationQueryResult& query) {

    if (!policy.enabled) {
        return {};
    }
    if (!policy.upstream_supports_pagination) {
        return {};
    }
    if (query.parse_error) {
        return {};
    }
    if (query.has_limit) {
        return {};
    }
    return { true, policy.default_limit };
}

} // namespace bytetaper::pagination
