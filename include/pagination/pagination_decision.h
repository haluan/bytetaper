// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "pagination/pagination_query.h"
#include "policy/pagination_policy.h"

namespace bytetaper::pagination {

struct PaginationDecision {
    bool should_apply_default_limit = false;
    bool should_cap_limit = false; // client limit exceeds max_limit
    std::uint32_t limit_to_apply = 0;
    const char* reason = nullptr; // static string; "limit_exceeds_max" or nullptr
};

// Returns a decision on whether to inject the policy default_limit.
// Injects only when: policy enabled, upstream supports pagination,
// no parse error, and the client supplied no explicit limit.
PaginationDecision make_pagination_decision(const policy::PaginationPolicy& policy,
                                            const PaginationQueryResult& query);

} // namespace bytetaper::pagination
