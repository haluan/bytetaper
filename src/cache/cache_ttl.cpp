// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_ttl.h"

namespace bytetaper::cache {

bool cache_ttl_valid(std::int64_t now_ms, std::int64_t expires_at_epoch_ms) {
    if (expires_at_epoch_ms <= 0) {
        return false;
    }
    return now_ms < expires_at_epoch_ms;
}

} // namespace bytetaper::cache
