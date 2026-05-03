// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/pagination_policy.h"

namespace bytetaper::policy {

const char* validate_pagination_policy_safe(const PaginationPolicy& policy) {
    if (!policy.enabled) {
        return nullptr;
    }

    const char* base_err = validate_pagination_policy(policy);
    if (base_err != nullptr) {
        return base_err;
    }

    if (policy.max_limit == 0) {
        return "pagination.max_limit required when pagination is enabled";
    }

    return nullptr;
}

} // namespace bytetaper::policy
