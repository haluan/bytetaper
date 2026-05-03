// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "api_intelligence/pagination_signal.h"

namespace bytetaper::api_intelligence {

PaginationRiskLevel assess_pagination_risk(const PaginationSignals& signals) {
    if (signals.oversized_response_warning_seen) {
        return PaginationRiskLevel::High;
    }
    if (signals.upstream_pagination_unsupported_seen && signals.missing_limit_seen) {
        return PaginationRiskLevel::High;
    }
    if (signals.max_limit_enforced_seen) {
        return PaginationRiskLevel::Medium;
    }
    return PaginationRiskLevel::Low;
}

} // namespace bytetaper::api_intelligence
