// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/compression_policy.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::policy {

TEST(CompressionPolicyTest, DisabledByDefault) {
    CompressionPolicy p{};
    EXPECT_FALSE(p.enabled);
    EXPECT_EQ(validate_compression_policy(p), nullptr);
}

TEST(CompressionPolicyTest, EnabledWithMinSize_Valid) {
    CompressionPolicy p{};
    p.enabled = true;
    p.min_size_bytes = 1024;
    EXPECT_EQ(validate_compression_policy(p), nullptr);
}

TEST(CompressionPolicyTest, MinSizeBytesStored) {
    CompressionPolicy p{};
    p.enabled = true;
    p.min_size_bytes = 512;
    EXPECT_EQ(p.min_size_bytes, 512u);
    EXPECT_EQ(validate_compression_policy(p), nullptr);
}

TEST(CompressionPolicyTest, ContentTypeAllowlistStored) {
    CompressionPolicy p{};
    p.enabled = true;
    std::strncpy(p.eligible_content_types[0], "application/json", kMaxContentTypeLen - 1);
    std::strncpy(p.eligible_content_types[1], "text/html", kMaxContentTypeLen - 1);
    p.eligible_content_type_count = 2;
    EXPECT_EQ(validate_compression_policy(p), nullptr);
    EXPECT_EQ(p.eligible_content_type_count, 2u);
}

TEST(CompressionPolicyTest, PreferredAlgorithmsStored) {
    CompressionPolicy p{};
    p.enabled = true;
    p.preferred_algorithms[0] = CompressionAlgorithm::Brotli;
    p.preferred_algorithms[1] = CompressionAlgorithm::Gzip;
    p.preferred_algorithm_count = 2;
    EXPECT_EQ(validate_compression_policy(p), nullptr);
    EXPECT_EQ(p.preferred_algorithms[0], CompressionAlgorithm::Brotli);
}

TEST(CompressionPolicyTest, AlreadyEncodedBehaviorStored) {
    CompressionPolicy p{};
    p.enabled = true;
    p.already_encoded_behavior = AlreadyEncodedBehavior::Passthrough;
    EXPECT_EQ(validate_compression_policy(p), nullptr);
    EXPECT_EQ(p.already_encoded_behavior, AlreadyEncodedBehavior::Passthrough);
}

TEST(CompressionPolicyTest, EmptyContentTypeList_Valid) {
    // Empty list = all content types eligible — not an error.
    CompressionPolicy p{};
    p.enabled = true;
    p.eligible_content_type_count = 0;
    EXPECT_EQ(validate_compression_policy(p), nullptr);
}

} // namespace bytetaper::policy
