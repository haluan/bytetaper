// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstdint>

namespace bytetaper::api_intelligence {

struct PaginationSignals {
    bool missing_limit_seen = false;
    bool max_limit_enforced_seen = false;
    bool oversized_response_warning_seen = false;
    bool upstream_pagination_unsupported_seen = false;
};

enum class PaginationRiskLevel : std::uint8_t { Low = 0, Medium, High };

// Pure function: maps observed signals to a risk level for API intelligence reporting.
// Priority: High > Medium > Low. HIGH when oversized warning is seen, or when
// upstream does not support pagination AND the client omitted the limit.
PaginationRiskLevel assess_pagination_risk(const PaginationSignals& signals);

} // namespace bytetaper::api_intelligence
