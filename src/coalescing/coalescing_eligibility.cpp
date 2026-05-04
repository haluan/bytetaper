// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_eligibility.h"

namespace bytetaper::coalescing {

std::string_view get_rejection_reason_string(CoalescingRejectionReason reason) {
    switch (reason) {
    case CoalescingRejectionReason::None:
        return "none";
    case CoalescingRejectionReason::MethodNotGet:
        return "method_not_get";
    }
    return "unknown";
}

CoalescingEligibility evaluate_coalescing_eligibility(policy::HttpMethod method) {
    if (method == policy::HttpMethod::Get) {
        return { true, CoalescingRejectionReason::None };
    }
    return { false, CoalescingRejectionReason::MethodNotGet };
}

} // namespace bytetaper::coalescing
