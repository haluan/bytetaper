// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_COALESCING_INFLIGHT_REGISTRY_H
#define BYTETAPER_COALESCING_INFLIGHT_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace bytetaper::coalescing {

/**
 * @brief Role assigned to a request by the registry.
 */
enum class InFlightRole : std::uint8_t {
    Leader = 0,   // First request for this key, will fetch from upstream.
    Follower = 1, // Subsequent request, should wait for leader.
    Reject = 2,   // Queue/Shard full, instantly synthesize error response.
};

/**
 * @brief Result of a request registration.
 */
struct RegistryRegistrationResult {
    InFlightRole role;
};

/**
 * @brief An entry in the in-flight request registry.
 * Following Orthodox C++ style: plain struct with fixed-size key buffer.
 */
struct InFlightEntry {
    char key[256] = { 0 };
    std::uint64_t created_at_epoch_ms = 0;
    std::uint64_t completed_at_epoch_ms = 0;
    std::uint32_t waiter_count = 0;
    bool active = false;
    bool completed = false;
};

/**
 * @brief Constants for sharded architecture.
 */
static constexpr std::size_t kInFlightShards = 256;
static constexpr std::size_t kSlotsPerShard = 16; // 4096 total capacity

/**
 * @brief A shard of the registry, protected by its own mutex to minimize contention.
 */
struct InFlightShard {
    std::mutex mutex;
    InFlightEntry slots[kSlotsPerShard];
};

/**
 * @brief Thread-safe, sharded, bounded in-memory registry for in-flight requests.
 * Uses lock striping to allow concurrent access to different shards.
 */
struct InFlightRegistry {
    InFlightShard shards[kInFlightShards];
};

/**
 * @brief Initializes the registry.
 *
 * @param registry The registry to initialize.
 */
void registry_init(InFlightRegistry* registry);

/**
 * @brief Registers a request in the registry.
 *
 * Determines if the request is a Leader, Follower, or should be Rejected.
 *
 * @param registry The registry instance.
 * @param key The coalescing key.
 * @param now_ms Current epoch time in milliseconds.
 * @param wait_window_ms Time window in which a request is considered "in flight".
 * @param max_waiters_per_key Maximum number of followers allowed per leader.
 * @return RegistryRegistrationResult The assigned role.
 */
RegistryRegistrationResult registry_register(InFlightRegistry* registry, const char* key,
                                             std::uint64_t now_ms, std::uint32_t wait_window_ms,
                                             std::uint32_t max_waiters_per_key);

/**
 * @brief Marks a leader's request as completed in the registry.
 *
 * If cacheable is true, the entry is kept in a "completed" state for a grace window.
 * If false, the entry is cleared immediately.
 *
 * @param registry The registry instance.
 * @param key The coalescing key.
 * @param cacheable Whether the response was stored in cache.
 * @param now_ms Current epoch time.
 */
void registry_complete(InFlightRegistry* registry, const char* key, bool cacheable,
                       std::uint64_t now_ms);

/**

 * @brief Deregisters a follower/waiter from the registry.
 *
 * Called when a follower times out or otherwise cancels its wait.
 *
 * @param registry The registry instance.
 * @param key The coalescing key.
 */
void registry_remove_waiter(InFlightRegistry* registry, const char* key);

} // namespace bytetaper::coalescing

#endif // BYTETAPER_COALESCING_INFLIGHT_REGISTRY_H
