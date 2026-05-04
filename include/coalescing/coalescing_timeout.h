// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_COALESCING_COALESCING_TIMEOUT_H
#define BYTETAPER_COALESCING_COALESCING_TIMEOUT_H

#include "coalescing/coalescing_decision.h"
#include "coalescing/inflight_registry.h"

namespace bytetaper::coalescing {

/**
 * @brief Handles fallback logic when a follower times out waiting for a leader.
 *
 * Deregisters the waiter from the registry to prevent slot exhaustion,
 * and mutates the decision to Bypass so the request continues upstream.
 *
 * @param registry The active in-flight registry.
 * @param decision The decision object to mutate.
 */
void handle_timeout_fallback(InFlightRegistry* registry, CoalescingDecision& decision);

} // namespace bytetaper::coalescing

#endif // BYTETAPER_COALESCING_COALESCING_TIMEOUT_H
