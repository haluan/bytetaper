// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "api_intelligence/coalescing_signal.h"

namespace bytetaper::api_intelligence {

CoalescingOpportunity assess_coalescing_opportunity(const CoalescingSignals& signals) {
    // If we've already seen a follower join, coalescing is already active and working.
    if (signals.follower_joined_seen) {
        return { false, nullptr, nullptr };
    }

    // If we see duplicate GET requests in a burst, and coalescing isn't working/active,
    // we recommend enabling it.
    if (signals.duplicate_get_seen) {
        return { true, "enable_request_coalescing", "duplicate_get_burst_seen" };
    }

    // Other signals like high fallback rate or too many waiters are recorded but don't
    // necessarily trigger an "enable" recommendation if duplicates aren't the primary issue
    // or if we're already failing for other reasons.
    return { false, nullptr, nullptr };
}

} // namespace bytetaper::api_intelligence
