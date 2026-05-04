// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/coalescing_follower_wait_stage.h"

#include "coalescing/wait_window.h"
#include "stages/l1_cache_lookup_stage.h"
#include "stages/l2_cache_lookup_stage.h"

#include <chrono>
#include <thread>

namespace bytetaper::stages {

apg::StageOutput coalescing_follower_wait_stage(apg::ApgTransformContext& context) {
    if (context.coalescing_decision.action != coalescing::CoalescingAction::Follower) {
        return { apg::StageResult::Continue };
    }

    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue };
    }

    const auto& policy = context.matched_policy->coalescing;
    std::uint32_t wait_window_ms = policy.wait_window_ms;
    std::uint64_t start_epoch_ms = static_cast<std::uint64_t>(context.request_epoch_ms);

    // Polling loop
    while (true) {
        // 1. Check L1 Cache
        apg::StageOutput l1_res = l1_cache_lookup_stage(context);
        if (l1_res.result == apg::StageResult::SkipRemaining) {
            return l1_res;
        }

        // 2. Check L2 Cache (if enabled)
        if (context.l2_cache != nullptr) {
            apg::StageOutput l2_res = l2_cache_lookup_stage(context);
            if (l2_res.result == apg::StageResult::SkipRemaining) {
                return l2_res;
            }
        }

        // 3. Check for timeout
        // We use a high-resolution steady clock for current time relative to start,
        // but the context request_epoch_ms is our base.
        // For simplicity and alignment with Orthodox C++ patterns in this project,
        // we'll use a mocked-safe "now" logic or standard system clock.
        auto now = std::chrono::system_clock::now();
        auto now_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        if (coalescing::has_wait_window_expired(start_epoch_ms, static_cast<std::uint64_t>(now_ms),
                                                wait_window_ms)) {
            break;
        }

        // 4. Wait a bit before polling again
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Fallback upstream on timeout/miss
    return { apg::StageResult::Continue };
}

} // namespace bytetaper::stages
