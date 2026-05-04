// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstdint>

namespace bytetaper::api_intelligence {

struct CoalescingSignals {
    bool duplicate_get_seen = false;
    bool follower_joined_seen = false;
    bool high_fallback_rate_seen = false;
    bool too_many_waiters_seen = false;
};

struct CoalescingOpportunity {
    bool has_opportunity = false;
    const char* recommendation_code = nullptr;
    const char* recommendation_reason = nullptr;
};

// Pure function: maps observed coalescing signals to an opportunity for API intelligence
// reporting.
CoalescingOpportunity assess_coalescing_opportunity(const CoalescingSignals& signals);

} // namespace bytetaper::api_intelligence
