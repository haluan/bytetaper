// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/coalescing_follower_wait_stage.h"

#include "coalescing/coalescing_timeout.h"
#include "coalescing/inflight_registry.h"
#include "coalescing/wait_window.h"
#include "metrics/coalescing_metrics.h"
#include "stages/l1_cache_lookup_stage.h"

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

    // Cache Integration Check
    if (policy.require_cache_enabled &&
        context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {

        // Follower bypasses waiting, and deregisters itself.
        if (context.coalescing_registry != nullptr) {
            coalescing::registry_remove_waiter(context.coalescing_registry,
                                               context.coalescing_decision.key);
        }
        context.coalescing_decision.action = coalescing::CoalescingAction::Bypass;
        record_coalescing_event(context.coalescing_metrics, metrics::CoalescingMetricEvent::Bypass);
        return { apg::StageResult::Continue, "cache-disabled-bypassed" };
    }

    std::uint32_t wait_window_ms = policy.wait_window_ms;
    std::uint64_t start_epoch_ms = static_cast<std::uint64_t>(context.request_epoch_ms);

    // Polling loop
    while (true) {
        // 1. Check L1 Cache
        apg::StageOutput l1_res = l1_cache_lookup_stage(context);
        if (l1_res.result == apg::StageResult::SkipRemaining) {
            record_coalescing_event(context.coalescing_metrics,
                                    metrics::CoalescingMetricEvent::FollowerCacheHit);
            return l1_res;
        }

        // 2. Check for timeout
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

        // 3. Wait a bit before polling again
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Fallback upstream on timeout/miss
    if (context.coalescing_registry != nullptr) {
        coalescing::handle_timeout_fallback(context.coalescing_registry,
                                            context.coalescing_decision);
        record_coalescing_event(context.coalescing_metrics,
                                metrics::CoalescingMetricEvent::Fallback);
    } else {
        context.coalescing_decision.action = coalescing::CoalescingAction::Bypass;
        record_coalescing_event(context.coalescing_metrics, metrics::CoalescingMetricEvent::Bypass);
    }
    return { apg::StageResult::Continue, "timeout-fallback" };
}

} // namespace bytetaper::stages
