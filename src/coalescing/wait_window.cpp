// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/wait_window.h"

namespace bytetaper::coalescing {

WaitWindowValidationResult validate_wait_window(std::uint32_t wait_window_ms) {
    if (wait_window_ms == 0) {
        return WaitWindowValidationResult::InvalidZero;
    }
    if (wait_window_ms > kMaxWaitWindowMs) {
        return WaitWindowValidationResult::InvalidTooLong;
    }
    return WaitWindowValidationResult::Valid;
}

std::string_view get_wait_fallback_reason_string(WaitFallbackReason reason) {
    switch (reason) {
    case WaitFallbackReason::Timeout:
        return "timeout";
    }
    return "unknown";
}

bool has_wait_window_expired(std::uint64_t start_epoch_ms, std::uint64_t now_epoch_ms,
                             std::uint32_t wait_window_ms) {
    // Check if current time is at or beyond the expiration threshold.
    // Standardizing on inclusive expiration: start + window == now is expired.
    return now_epoch_ms >= (start_epoch_ms + static_cast<std::uint64_t>(wait_window_ms));
}

} // namespace bytetaper::coalescing
