// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"

namespace bytetaper::cache {

bool is_expired(const CacheEntry& entry, std::int64_t now_ms) {
    if (entry.expires_at_epoch_ms == 0) {
        return false;
    }
    return now_ms >= entry.expires_at_epoch_ms;
}

} // namespace bytetaper::cache
