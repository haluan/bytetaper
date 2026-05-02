// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_ttl.h"

#include <gtest/gtest.h>

namespace bytetaper::cache {

TEST(CacheTtlTest, ValidEntryAccepted) {
    EXPECT_TRUE(cache_ttl_valid(1000, 2000));
}

TEST(CacheTtlTest, ExpiredEntryRejected) {
    EXPECT_FALSE(cache_ttl_valid(2000, 2000)); // at boundary: now >= expires
}

TEST(CacheTtlTest, ZeroExpiryRejected) {
    EXPECT_FALSE(cache_ttl_valid(1000, 0));
}

TEST(CacheTtlTest, NegativeExpiryRejected) {
    EXPECT_FALSE(cache_ttl_valid(1000, -1));
}

} // namespace bytetaper::cache
