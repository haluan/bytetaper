// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/coalescing_leader_completion_stage.h"

#include "coalescing/inflight_registry.h"
#include "policy/route_policy.h"

#include <chrono>

namespace bytetaper::stages {

apg::StageOutput coalescing_leader_completion_stage(apg::ApgTransformContext& context) {
    // Only leaders need to signal completion
    if (context.coalescing_decision.action != coalescing::CoalescingAction::Leader) {
        return { apg::StageResult::Continue, "not-leader" };
    }

    if (context.coalescing_registry == nullptr) {
        return { apg::StageResult::Continue, "no-registry" };
    }

    // Determine if the response was cacheable.
    // If it was, we keep the entry in the registry for a grace period so new requests join as
    // followers.
    bool cacheable = false;
    if (context.matched_policy != nullptr &&
        context.matched_policy->cache.behavior == policy::CacheBehavior::Store) {
        // Status code 2xx is generally cacheable in our system
        if (context.response_status_code >= 200 && context.response_status_code < 300) {
            cacheable = true;
        }
    }

    // Capture completion time
    auto now = std::chrono::system_clock::now();
    std::uint64_t now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    coalescing::registry_complete(context.coalescing_registry, context.coalescing_decision.key,
                                  cacheable, now_ms);

    return { apg::StageResult::Continue, cacheable ? "completed-cacheable" : "completed-cleared" };
}

} // namespace bytetaper::stages
