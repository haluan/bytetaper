// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "coalescing/coalescing_timeout.h"

#include <gtest/gtest.h>

namespace bytetaper::coalescing {

class CoalescingTimeoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_init(&registry);
    }

    InFlightRegistry registry;
};

TEST_F(CoalescingTimeoutTest, FallbackMutatesDecisionAndDeregisters) {
    const char* key = "c_key:test:1:/path";
    std::uint64_t now = 1000;
    std::uint32_t window = 100;
    std::uint32_t max_waiters = 1;

    // 1. Setup a Leader and a Follower in the registry
    registry_register(&registry, key, now, window, max_waiters); // Leader
    registry_register(&registry, key, now, window, max_waiters); // Follower (waiter_count = 1)

    // Verify it's full for new followers
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Reject);

    CoalescingDecision decision;
    decision.action = CoalescingAction::Follower;
    std::strcpy(decision.key, key);

    // 2. Execute timeout fallback
    handle_timeout_fallback(&registry, decision);

    // 3. Verify Decision Mutation
    EXPECT_EQ(decision.action, CoalescingAction::Bypass);
    EXPECT_EQ(decision.reason, CoalescingDecisionReason::WaitWindowExpired);
    EXPECT_STREQ(get_decision_reason_string(decision.reason).data(), "wait_window_expired");

    // 4. Verify Registry Cleanup (New follower can join now)
    EXPECT_EQ(registry_register(&registry, key, now, window, max_waiters).role,
              InFlightRole::Follower);
}

} // namespace bytetaper::coalescing
