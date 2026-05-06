// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/coalescing_follower_wait_stage.h"

#include "coalescing/coalescing_timeout.h"
#include "coalescing/inflight_registry.h"
#include "metrics/coalescing_metrics.h"
#include "stages/l1_cache_lookup_stage.h"

#include <chrono>

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

    const std::uint32_t wait_window_ms = policy.wait_window_ms;
    if (context.coalescing_registry == nullptr) {
        context.coalescing_decision.action = coalescing::CoalescingAction::Bypass;
        record_coalescing_event(context.coalescing_metrics, metrics::CoalescingMetricEvent::Bypass);
        return { apg::StageResult::Continue, "no-registry" };
    }

    // Step 1: L1 Check before waiting
    apg::StageOutput l1_res = l1_cache_lookup_stage(context);
    if (l1_res.result == apg::StageResult::SkipRemaining) {
        if (context.coalescing_registry != nullptr) {
            coalescing::registry_remove_waiter(context.coalescing_registry,
                                               context.coalescing_decision.key);
        }
        record_coalescing_event(context.coalescing_metrics,
                                metrics::CoalescingMetricEvent::FollowerCacheHit);
        return l1_res;
    }

    // Step 2: Block until leader completes or timeout.
    // This uses a non-polling, notification-aware wait strategy via std::condition_variable
    // inside registry_wait_for_completion, avoiding any sleep-based polling loops.
    const coalescing::RegistryWaitResult wait_result = coalescing::registry_wait_for_completion(
        context.coalescing_registry, context.coalescing_decision.key, wait_window_ms);

    // Step 3: L1 check after wake (whether Completed, Timeout, or Missing)
    l1_res = l1_cache_lookup_stage(context);
    if (l1_res.result == apg::StageResult::SkipRemaining) {
        if (context.coalescing_registry != nullptr) {
            coalescing::registry_remove_waiter(context.coalescing_registry,
                                               context.coalescing_decision.key);
        }
        record_coalescing_event(context.coalescing_metrics,
                                metrics::CoalescingMetricEvent::FollowerCacheHit);
        return l1_res;
    }

    // L1 miss after wait — fallback upstream
    if (wait_result == coalescing::RegistryWaitResult::Timeout ||
        wait_result == coalescing::RegistryWaitResult::Missing) {
        coalescing::handle_timeout_fallback(context.coalescing_registry,
                                            context.coalescing_decision);
        record_coalescing_event(context.coalescing_metrics,
                                metrics::CoalescingMetricEvent::Fallback);
        return { apg::StageResult::Continue, "timeout-fallback" };
    }

    // Leader completed (Completed) but L1 still miss — leader may not have cached or was evicted.
    coalescing::handle_timeout_fallback(context.coalescing_registry, context.coalescing_decision);
    record_coalescing_event(context.coalescing_metrics, metrics::CoalescingMetricEvent::Fallback);
    return { apg::StageResult::Continue, "leader-complete-l1-miss" };
}

} // namespace bytetaper::stages
