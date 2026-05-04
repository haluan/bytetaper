// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_COALESCING_COALESCING_SAFETY_H
#define BYTETAPER_COALESCING_COALESCING_SAFETY_H

#include "coalescing/coalescing_eligibility.h"

namespace bytetaper::coalescing {

/**
 * @brief Input for coalescing safety evaluation.
 *
 * We use boolean flags to indicate the presence of sensitive headers
 * to avoid passing raw tokens across module boundaries.
 */
struct CoalescingSafetyInput {
    bool has_authorization_header = false;
    bool has_cookie_header = false;
    bool allow_authenticated = false;
};

/**
 * @brief Evaluates if a request is safe to coalesce given its authentication state.
 *
 * Authenticated requests bypass coalescing unless explicit policy allows it.
 *
 * @param input The safety evaluation input.
 * @return CoalescingEligibility The safety result.
 */
CoalescingEligibility evaluate_coalescing_safety(const CoalescingSafetyInput& input);

} // namespace bytetaper::coalescing

#endif // BYTETAPER_COALESCING_COALESCING_SAFETY_H
