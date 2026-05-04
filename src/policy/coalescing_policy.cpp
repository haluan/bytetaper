// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/coalescing_policy.h"

namespace bytetaper::policy {

const char* validate_coalescing_policy(const CoalescingPolicy& policy) {
    if (!policy.enabled) {
        return nullptr;
    }

    if (policy.wait_window_ms == 0) {
        return "coalescing.wait_window_ms must be > 0";
    }

    if (policy.wait_window_ms > 5000) {
        return "coalescing.wait_window_ms exceeds maximum allowed wait (5000ms)";
    }

    if (policy.max_waiters_per_key == 0) {
        return "coalescing.max_waiters_per_key must be > 0";
    }

    return nullptr;
}

} // namespace bytetaper::policy
