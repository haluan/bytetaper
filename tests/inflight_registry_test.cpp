// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/inflight_registry.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::coalescing {

class InFlightRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_init(&registry);
    }

    InFlightRegistry registry;
};

TEST_F(InFlightRegistryTest, LeaderFollowerBasic) {
    const char* key = "test-key";
    std::uint64_t now = 1000;
    std::uint32_t window = 100;
    std::uint32_t max_waiters = 5;

    // First request is Leader
    auto res1 = registry_register(&registry, key, now, window, max_waiters);
    EXPECT_EQ(res1.role, InFlightRole::Leader);

    // Second request is Follower
    auto res2 = registry_register(&registry, key, now + 1, window, max_waiters);
    EXPECT_EQ(res2.role, InFlightRole::Follower);
}

TEST_F(InFlightRegistryTest, DifferentKeysAreLeaders) {
    std::uint64_t now = 1000;
    std::uint32_t window = 100;
    std::uint32_t max_waiters = 5;

    EXPECT_EQ(registry_register(&registry, "key1", now, window, max_waiters).role,
              InFlightRole::Leader);
    EXPECT_EQ(registry_register(&registry, "key2", now, window, max_waiters).role,
              InFlightRole::Leader);
}

TEST_F(InFlightRegistryTest, ExpiryBecomesLeader) {
    const char* key = "test-key";
    std::uint64_t now = 1000;
    std::uint32_t window = 100;
    std::uint32_t max_waiters = 5;

    registry_register(&registry, key, now, window, max_waiters);

    // At now + 100, it should be expired (>= 1000 + 100)
    auto res = registry_register(&registry, key, now + 100, window, max_waiters);
    EXPECT_EQ(res.role, InFlightRole::Leader);
}

TEST_F(InFlightRegistryTest, MaxWaitersEnforcedFastFail) {
    const char* key = "test-key";
    std::uint64_t now = 1000;
    std::uint32_t window = 100;
    std::uint32_t max_waiters = 2;

    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Leader);
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Follower); // waiter 1
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Follower); // waiter 2

    // Third follower should be Rejected (Fast-Fail)
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Reject);
}

TEST_F(InFlightRegistryTest, CompletionAllowsNewLeader) {
    const char* key = "test-key";
    std::uint64_t now = 1000;
    std::uint32_t window = 100;
    std::uint32_t max_waiters = 5;

    registry_register(&registry, key, now, window, max_waiters);
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Follower);

    registry_complete(&registry, key);

    // After completion, next request is Leader again
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Leader);
}

TEST_F(InFlightRegistryTest, ShardFullRejects) {
    auto hash_fn = [](const char* s) -> std::uint64_t {
        std::uint64_t hash = 14695981039346656037ULL;
        while (*s) {
            hash ^= static_cast<std::uint64_t>(*s++);
            hash *= 1099511628211ULL;
        }
        return hash;
    };

    std::uint32_t window = 100;
    std::uint32_t max_waiters = 5;

    // Find keys that hash to the same shard (index 0)
    char keys[kSlotsPerShard][32];
    int found = 0;
    for (int i = 0; found < (int) kSlotsPerShard; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "key%d", i);
        if ((hash_fn(buf) % kInFlightShards) == 0) {
            strcpy(keys[found], buf);
            found++;
        }
    }

    // Fill the shard
    for (int i = 0; i < (int) kSlotsPerShard; ++i) {
        EXPECT_EQ(registry_register(&registry, keys[i], 1000, window, max_waiters).role,
                  InFlightRole::Leader);
    }

    // 17th key to the same shard should be Rejected
    for (int i = 0;; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "extra%d", i);
        if ((hash_fn(buf) % kInFlightShards) == 0) {
            EXPECT_EQ(registry_register(&registry, buf, 1000, window, max_waiters).role,
                      InFlightRole::Reject);
            break;
        }
    }
}

} // namespace bytetaper::coalescing
