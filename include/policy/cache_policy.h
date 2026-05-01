// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_POLICY_CACHE_POLICY_H
#define BYTETAPER_POLICY_CACHE_POLICY_H

#include <cstdint>

namespace bytetaper::policy {

enum class CacheBehavior : std::uint8_t {
    Default = 0, // no explicit cache instruction (pass-through)
    Bypass = 1,  // always bypass cache
    Store = 2,   // store response in cache
};

struct CachePolicy {
    CacheBehavior behavior = CacheBehavior::Default;
    std::uint32_t ttl_seconds = 0; // 0 = use upstream headers / server default
};

} // namespace bytetaper::policy

#endif // BYTETAPER_POLICY_CACHE_POLICY_H
