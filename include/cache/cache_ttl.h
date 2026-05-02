// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_CACHE_CACHE_TTL_H
#define BYTETAPER_CACHE_CACHE_TTL_H

#include <cstdint>

namespace bytetaper::cache {

// Returns true if the entry is still valid at now_ms.
// Returns false if expires_at_epoch_ms <= 0 (no TTL = treated as expired)
// or if now_ms >= expires_at_epoch_ms.
bool cache_ttl_valid(std::int64_t now_ms, std::int64_t expires_at_epoch_ms);

} // namespace bytetaper::cache

#endif // BYTETAPER_CACHE_CACHE_TTL_H
