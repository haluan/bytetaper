// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_COALESCING_POLICY_VALIDATOR_H
#define BYTETAPER_POLICY_COALESCING_POLICY_VALIDATOR_H

#include "policy/coalescing_policy.h"

namespace bytetaper::policy {

struct CachePolicy;

// Stricter validator for production use.
const char* validate_coalescing_policy_safe(const CoalescingPolicy& policy,
                                            const CachePolicy* cache_policy = nullptr);

} // namespace bytetaper::policy

#endif // BYTETAPER_POLICY_COALESCING_POLICY_VALIDATOR_H
