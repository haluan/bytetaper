// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "runtime/pending_lookup_registry.h"

#include <cstring>

namespace bytetaper::runtime {

void pending_lookup_init(PendingLookupRegistry* reg) {
    if (reg == nullptr)
        return;
    for (std::size_t i = 0; i < kPendingLookupMaxKeys; ++i) {
        reg->occupied[i] = false;
    }
    reg->count = 0;
}

bool pending_lookup_try_mark(PendingLookupRegistry* reg, const char* key) {
    if (reg == nullptr || key == nullptr)
        return false;

    // 1. Check for duplicates
    for (std::size_t i = 0; i < kPendingLookupMaxKeys; ++i) {
        if (reg->occupied[i] && std::strcmp(reg->keys[i], key) == 0) {
            return false;
        }
    }

    // 2. Find empty slot
    if (reg->count >= kPendingLookupMaxKeys) {
        return false;
    }

    for (std::size_t i = 0; i < kPendingLookupMaxKeys; ++i) {
        if (!reg->occupied[i]) {
            std::strncpy(reg->keys[i], key, cache::kCacheKeyMaxLen - 1);
            reg->keys[i][cache::kCacheKeyMaxLen - 1] = '\0';
            reg->occupied[i] = true;
            reg->count++;
            return true;
        }
    }

    return false;
}

void pending_lookup_clear(PendingLookupRegistry* reg, const char* key) {
    if (reg == nullptr || key == nullptr)
        return;

    for (std::size_t i = 0; i < kPendingLookupMaxKeys; ++i) {
        if (reg->occupied[i] && std::strcmp(reg->keys[i], key) == 0) {
            reg->occupied[i] = false;
            if (reg->count > 0)
                reg->count--;
            return;
        }
    }
}

} // namespace bytetaper::runtime
