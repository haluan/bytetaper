// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_decision.h"

namespace bytetaper::coalescing {

std::string_view get_decision_reason_string(CoalescingDecisionReason reason) {
    switch (reason) {
    case CoalescingDecisionReason::PolicyDisabled:
        return "policy_disabled";
    case CoalescingDecisionReason::MethodNotGet:
        return "method_not_get";
    case CoalescingDecisionReason::AuthenticatedRequest:
        return "authenticated_request";
    case CoalescingDecisionReason::MissingKey:
        return "missing_key";
    case CoalescingDecisionReason::LeaderCreated:
        return "leader_created";
    case CoalescingDecisionReason::FollowerJoined:
        return "follower_joined";
    case CoalescingDecisionReason::TooManyWaiters:
        return "too_many_waiters";
    }
    return "unknown";
}

CoalescingDecision evaluate_coalescing_decision(InFlightRegistry* registry,
                                                const CoalescingDecisionContext& context) {
    CoalescingDecision decision;

    // 1. Policy Check
    if (context.policy == nullptr || !context.policy->enabled) {
        decision.action = CoalescingAction::Bypass;
        decision.reason = CoalescingDecisionReason::PolicyDisabled;
        return decision;
    }

    // 2. Eligibility Check (Method)
    CoalescingEligibility eligibility = evaluate_coalescing_eligibility(context.method);
    if (!eligibility.is_eligible) {
        decision.action = CoalescingAction::Bypass;
        decision.reason = CoalescingDecisionReason::MethodNotGet;
        return decision;
    }

    // 3. Safety Check (Authentication)
    CoalescingEligibility safety = evaluate_coalescing_safety(context.safety_input);
    if (!safety.is_eligible) {
        decision.action = CoalescingAction::Bypass;
        decision.reason = CoalescingDecisionReason::AuthenticatedRequest;
        return decision;
    }

    // 4. Key Construction
    if (!build_coalescing_key(context.key_input, decision.key, sizeof(decision.key))) {
        decision.action = CoalescingAction::Bypass;
        decision.reason = CoalescingDecisionReason::MissingKey;
        return decision;
    }

    // 5. Registry Registration
    RegistryRegistrationResult reg_res =
        registry_register(registry, decision.key, context.now_ms, context.policy->wait_window_ms,
                          context.policy->max_waiters_per_key);

    // 6. Mapping Registry Result
    switch (reg_res.role) {
    case InFlightRole::Leader:
        decision.action = CoalescingAction::Leader;
        decision.reason = CoalescingDecisionReason::LeaderCreated;
        break;
    case InFlightRole::Follower:
        decision.action = CoalescingAction::Follower;
        decision.reason = CoalescingDecisionReason::FollowerJoined;
        break;
    case InFlightRole::Reject:
        decision.action = CoalescingAction::Reject;
        decision.reason = CoalescingDecisionReason::TooManyWaiters;
        break;
    }

    return decision;
}

} // namespace bytetaper::coalescing
