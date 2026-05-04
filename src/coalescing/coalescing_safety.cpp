// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_safety.h"

namespace bytetaper::coalescing {

CoalescingEligibility evaluate_coalescing_safety(const CoalescingSafetyInput& input) {
    // If policy explicitly allows authenticated coalescing, it is eligible.
    if (input.allow_authenticated) {
        return { true, CoalescingRejectionReason::None };
    }

    // By default, any sign of authentication (Authorization or Cookie headers)
    // makes the request ineligible for coalescing to prevent safe GET leaks
    // between users.
    if (input.has_authorization_header || input.has_cookie_header) {
        return { false, CoalescingRejectionReason::AuthenticatedRequest };
    }

    return { true, CoalescingRejectionReason::None };
}

} // namespace bytetaper::coalescing
