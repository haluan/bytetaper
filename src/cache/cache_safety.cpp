// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_safety.h"

namespace bytetaper::cache {

bool cache_auth_bypass(bool has_authorization, bool has_cookie, bool private_cache_enabled) {
    // If the route explicitly opts in to private caching, we don't bypass.
    if (private_cache_enabled) {
        return false;
    }

    // Default safety: bypass cache if any authentication headers are present.
    return has_authorization || has_cookie;
}

} // namespace bytetaper::cache
