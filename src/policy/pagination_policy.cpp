// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/pagination_policy.h"

namespace bytetaper::policy {

const char* validate_pagination_policy(const PaginationPolicy& policy) {
    if (!policy.enabled) {
        return nullptr;
    }
    if (policy.mode == PaginationMode::None) {
        return "pagination.mode required when pagination is enabled";
    }
    if (policy.default_limit == 0) {
        return "pagination.default_limit required when pagination is enabled";
    }
    if (policy.max_limit > 0 && policy.max_limit < policy.default_limit) {
        return "pagination.max_limit must not be less than default_limit";
    }
    if (policy.limit_param[0] == '\0') {
        return "pagination.limit_param must not be empty";
    }
    if (policy.offset_param[0] == '\0') {
        return "pagination.offset_param must not be empty";
    }
    return nullptr;
}

} // namespace bytetaper::policy
