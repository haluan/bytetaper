// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_RUNTIME_PENDING_LOOKUP_REGISTRY_H
#define BYTETAPER_RUNTIME_PENDING_LOOKUP_REGISTRY_H

#include "cache/cache_entry.h"

#include <cstddef>
#include <mutex>

namespace bytetaper::runtime {

static constexpr std::size_t kPendingLookupMaxKeys = 1024;

struct PendingLookupRegistry {
    std::mutex mu;
    char keys[kPendingLookupMaxKeys][cache::kCacheKeyMaxLen];
    bool occupied[kPendingLookupMaxKeys];
    std::size_t count;
};

void pending_lookup_init(PendingLookupRegistry* reg);

// Marks key as pending. Returns true if newly marked; false if already present or full.
bool pending_lookup_try_mark(PendingLookupRegistry* reg, const char* key);

// Clears the pending marker for key (call after worker completes, errors, or drops).
void pending_lookup_clear(PendingLookupRegistry* reg, const char* key);

} // namespace bytetaper::runtime

#endif // BYTETAPER_RUNTIME_PENDING_LOOKUP_REGISTRY_H
