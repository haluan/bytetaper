// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/inflight_registry.h"

#include <cstring>

namespace bytetaper::coalescing {

namespace {

/**
 * @brief Simple FNV-1a hash for strings.
 */
std::uint64_t hash_string(const char* s) {
    std::uint64_t hash = 14695981039346656037ULL;
    if (s == nullptr)
        return hash;
    while (*s) {
        hash ^= static_cast<std::uint64_t>(*s++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace

void registry_init(InFlightRegistry* registry) {
    if (registry == nullptr)
        return;
    for (std::size_t i = 0; i < kInFlightShards; ++i) {
        std::lock_guard<std::mutex> lock(registry->shards[i].mutex);
        for (std::size_t j = 0; j < kSlotsPerShard; ++j) {
            registry->shards[i].slots[j].active = false;
        }
    }
}

RegistryRegistrationResult registry_register(InFlightRegistry* registry, const char* key,
                                             std::uint64_t now_ms, std::uint32_t wait_window_ms,
                                             std::uint32_t max_waiters_per_key) {
    if (registry == nullptr || key == nullptr) {
        return { InFlightRole::Reject };
    }

    std::uint64_t hash = hash_string(key);
    std::size_t shard_idx = hash % kInFlightShards;
    InFlightShard& shard = registry->shards[shard_idx];

    std::lock_guard<std::mutex> lock(shard.mutex);

    // Linear probing within the shard's slots
    for (std::size_t j = 0; j < kSlotsPerShard; ++j) {
        InFlightEntry& slot = shard.slots[j];

        // Check for existing active entry
        if (slot.active && std::strcmp(slot.key, key) == 0) {
            // Check if expired
            if (now_ms >= slot.created_at_epoch_ms + wait_window_ms) {
                // Treat as new leader
                std::strncpy(slot.key, key, sizeof(slot.key) - 1);
                slot.created_at_epoch_ms = now_ms;
                slot.waiter_count = 0;
                slot.active = true;
                return { InFlightRole::Leader };
            }

            // Still in flight, check waiter limit
            if (slot.waiter_count < max_waiters_per_key) {
                slot.waiter_count++;
                return { InFlightRole::Follower };
            } else {
                // [BT-130-005] Too many waiters, Reject to protect backend
                return { InFlightRole::Reject };
            }
        }
    }

    // Try to find an empty or reusable slot in the shard
    for (std::size_t j = 0; j < kSlotsPerShard; ++j) {
        InFlightEntry& slot = shard.slots[j];
        if (!slot.active) {
            std::strncpy(slot.key, key, sizeof(slot.key) - 1);
            slot.created_at_epoch_ms = now_ms;
            slot.waiter_count = 0;
            slot.active = true;
            return { InFlightRole::Leader };
        }
    }

    // [BT-130-005] Shard full (shield full), instantly drop traffic
    return { InFlightRole::Reject };
}

void registry_complete(InFlightRegistry* registry, const char* key) {
    if (registry == nullptr || key == nullptr) {
        return;
    }

    std::uint64_t hash = hash_string(key);
    std::size_t shard_idx = hash % kInFlightShards;
    InFlightShard& shard = registry->shards[shard_idx];

    std::lock_guard<std::mutex> lock(shard.mutex);

    for (std::size_t j = 0; j < kSlotsPerShard; ++j) {
        InFlightEntry& slot = shard.slots[j];
        if (slot.active && std::strcmp(slot.key, key) == 0) {
            slot.active = false;
            return;
        }
    }
}

} // namespace bytetaper::coalescing
