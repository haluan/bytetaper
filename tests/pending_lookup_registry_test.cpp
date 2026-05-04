// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "runtime/pending_lookup_registry.h"

#include <gtest/gtest.h>
#include <string>

namespace bytetaper::runtime {

class PendingLookupRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        pending_lookup_init(&reg);
    }
    PendingLookupRegistry reg;
};

TEST_F(PendingLookupRegistryTest, TryMarkNewKey) {
    EXPECT_TRUE(pending_lookup_try_mark(&reg, "key1"));
    EXPECT_EQ(reg.count, 1);
}

TEST_F(PendingLookupRegistryTest, TryMarkDuplicateKey) {
    EXPECT_TRUE(pending_lookup_try_mark(&reg, "key1"));
    EXPECT_FALSE(pending_lookup_try_mark(&reg, "key1"));
    EXPECT_EQ(reg.count, 1);
}

TEST_F(PendingLookupRegistryTest, ClearAndRemarkKey) {
    EXPECT_TRUE(pending_lookup_try_mark(&reg, "key1"));
    pending_lookup_clear(&reg, "key1");
    EXPECT_EQ(reg.count, 0);
    EXPECT_TRUE(pending_lookup_try_mark(&reg, "key1"));
}

TEST_F(PendingLookupRegistryTest, FullRegistryReturnsFalse) {
    for (std::size_t i = 0; i < kPendingLookupMaxKeys; ++i) {
        std::string key = "key" + std::to_string(i);
        EXPECT_TRUE(pending_lookup_try_mark(&reg, key.c_str()));
    }
    EXPECT_EQ(reg.count, kPendingLookupMaxKeys);
    EXPECT_FALSE(pending_lookup_try_mark(&reg, "one_more"));
}

TEST_F(PendingLookupRegistryTest, ClearUnknownKeyNoOp) {
    EXPECT_TRUE(pending_lookup_try_mark(&reg, "key1"));
    pending_lookup_clear(&reg, "unknown");
    EXPECT_EQ(reg.count, 1);
}

} // namespace bytetaper::runtime
