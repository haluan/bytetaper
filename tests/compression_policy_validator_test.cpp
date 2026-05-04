// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/compression_policy.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::policy {

static CompressionPolicy make_valid() {
    CompressionPolicy p{};
    p.enabled = true;
    p.min_size_bytes = 1024;

    std::strcpy(p.eligible_content_types[0], "application/json");
    p.eligible_content_type_count = 1;

    p.preferred_algorithms[0] = CompressionAlgorithm::Gzip;
    p.preferred_algorithm_count = 1;

    p.already_encoded_behavior = AlreadyEncodedBehavior::Skip;
    return p;
}

TEST(CompressionPolicyValidatorTest, ValidPolicy_Passes) {
    auto p = make_valid();
    EXPECT_EQ(validate_compression_policy_safe(p), nullptr);
}

TEST(CompressionPolicyValidatorTest, DisabledPolicy_PassesEvenIfInvalid) {
    CompressionPolicy p{};
    p.enabled = false;
    p.min_size_bytes = 0; // would fail if enabled
    EXPECT_EQ(validate_compression_policy_safe(p), nullptr);
}

TEST(CompressionPolicyValidatorTest, ZeroMinSize_Fails) {
    auto p = make_valid();
    p.min_size_bytes = 0;
    EXPECT_STREQ(validate_compression_policy_safe(p),
                 "compression enabled with min_size_bytes <= 0");
}

TEST(CompressionPolicyValidatorTest, EmptyContentTypes_Fails) {
    auto p = make_valid();
    p.eligible_content_type_count = 0;
    EXPECT_STREQ(validate_compression_policy_safe(p),
                 "compression enabled with empty eligible_content_types");
}

TEST(CompressionPolicyValidatorTest, EmptyAlgorithms_Fails) {
    auto p = make_valid();
    p.preferred_algorithm_count = 0;
    EXPECT_STREQ(validate_compression_policy_safe(p),
                 "compression enabled with no preferred_algorithms");
}

TEST(CompressionPolicyValidatorTest, NoneAlgorithm_Fails) {
    auto p = make_valid();
    p.preferred_algorithms[0] = CompressionAlgorithm::None;
    EXPECT_STREQ(validate_compression_policy_safe(p),
                 "compression.preferred_algorithms contains 'none'");
}

TEST(CompressionPolicyValidatorTest, AlreadyEncodedPassthrough_Fails) {
    auto p = make_valid();
    p.already_encoded_behavior = AlreadyEncodedBehavior::Passthrough;
    EXPECT_STREQ(validate_compression_policy_safe(p),
                 "compression.already_encoded_behavior must be 'skip'");
}

TEST(CompressionPolicyValidatorTest, StructuralChecksDelegated) {
    auto p = make_valid();
    p.eligible_content_type_count = kMaxEligibleContentTypes + 1;
    EXPECT_STREQ(validate_compression_policy_safe(p),
                 "compression.eligible_content_types exceeds maximum count");
}

} // namespace bytetaper::policy
