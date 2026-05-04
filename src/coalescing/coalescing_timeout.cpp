// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_timeout.h"

namespace bytetaper::coalescing {

void handle_timeout_fallback(InFlightRegistry* registry, CoalescingDecision& decision) {
    if (registry == nullptr) {
        return;
    }

    // 1. Deregister the waiter to free up queue slots
    registry_remove_waiter(registry, decision.key);

    // 2. Mutate decision to bypass coalescing for this request
    decision.action = CoalescingAction::Bypass;
    decision.reason = CoalescingDecisionReason::WaitWindowExpired;
}

} // namespace bytetaper::coalescing
