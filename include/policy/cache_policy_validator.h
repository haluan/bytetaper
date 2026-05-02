// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "policy/cache_policy.h"
#include "policy/route_policy.h"

namespace bytetaper::policy {

// Runs structural checks from validate_cache_policy() plus route-level safety checks.
// Returns nullptr on success, or a static error string on violation.
const char* validate_cache_policy_safe(const CachePolicy& policy, HttpMethod route_method);

} // namespace bytetaper::policy
