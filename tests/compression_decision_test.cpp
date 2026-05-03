// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/compression_decision.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::compression {

static policy::CompressionPolicy make_policy() {
    policy::CompressionPolicy p{};
    p.enabled = true;
    p.min_size_bytes = 512;
    std::strncpy(p.eligible_content_types[0], "application/json", policy::kMaxContentTypeLen - 1);
    p.eligible_content_type_count = 1;
    return p;
}

static CompressionDecisionInput make_eligible_input(const policy::CompressionPolicy* pol) {
    CompressionDecisionInput in{};
    in.compression_policy = pol;
    in.client_encoding.supports_gzip = true;
    in.client_encoding.supports_br = true;
    in.status_code = 200;
    in.response_encoding.already_encoded = false;
    in.content_type = "application/json";
    in.content_type_len = std::strlen("application/json");
    in.body_len = 1024;
    in.body_size_known = true;
    return in;
}

TEST(CompressionDecisionTest, EligibleResponse_Candidate) {
    auto pol = make_policy();
    auto in = make_eligible_input(&pol);
    auto d = make_compression_decision(in);
    EXPECT_TRUE(d.candidate);
    EXPECT_EQ(d.reason, nullptr);
    EXPECT_EQ(d.selected_algorithm_hint, policy::CompressionAlgorithm::Brotli);
}

TEST(CompressionDecisionTest, PolicyDisabled_NotCandidate) {
    auto pol = make_policy();
    pol.enabled = false;
    auto in = make_eligible_input(&pol);
    auto d = make_compression_decision(in);
    EXPECT_FALSE(d.candidate);
    EXPECT_STREQ(d.reason, "policy_disabled");
}

TEST(CompressionDecisionTest, NoClientSupport_NotCandidate) {
    auto pol = make_policy();
    auto in = make_eligible_input(&pol);
    in.client_encoding = {}; // all false
    auto d = make_compression_decision(in);
    EXPECT_FALSE(d.candidate);
    EXPECT_STREQ(d.reason, "no_client_support");
}

TEST(CompressionDecisionTest, AlreadyEncoded_NotCandidate) {
    auto pol = make_policy();
    auto in = make_eligible_input(&pol);
    in.response_encoding.already_encoded = true;
    auto d = make_compression_decision(in);
    EXPECT_FALSE(d.candidate);
    EXPECT_STREQ(d.reason, "already_encoded");
}

TEST(CompressionDecisionTest, Non2xx_NotCandidate) {
    auto pol = make_policy();
    auto in = make_eligible_input(&pol);
    in.status_code = 404;
    auto d = make_compression_decision(in);
    EXPECT_FALSE(d.candidate);
    EXPECT_STREQ(d.reason, "non_2xx_status");
}

TEST(CompressionDecisionTest, ContentTypeNotEligible_NotCandidate) {
    auto pol = make_policy();
    auto in = make_eligible_input(&pol);
    in.content_type = "image/png";
    in.content_type_len = std::strlen("image/png");
    auto d = make_compression_decision(in);
    EXPECT_FALSE(d.candidate);
    EXPECT_STREQ(d.reason, "content_type_not_eligible");
}

TEST(CompressionDecisionTest, TooSmall_NotCandidate) {
    auto pol = make_policy();
    auto in = make_eligible_input(&pol);
    in.body_len = 100; // below min_size_bytes=512
    auto d = make_compression_decision(in);
    EXPECT_FALSE(d.candidate);
    EXPECT_STREQ(d.reason, "below_minimum");
}

TEST(CompressionDecisionTest, ReasonsStable) {
    auto pol = make_policy();
    pol.enabled = false;
    auto in = make_eligible_input(&pol);
    auto d1 = make_compression_decision(in);
    auto d2 = make_compression_decision(in);
    EXPECT_STREQ(d1.reason, d2.reason);
}

} // namespace bytetaper::compression
