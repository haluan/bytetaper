// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/compression_eligibility.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::compression {

static policy::CompressionPolicy make_policy(std::initializer_list<const char*> types) {
    policy::CompressionPolicy p{};
    p.enabled = true;
    for (const char* t : types) {
        if (p.eligible_content_type_count < policy::kMaxEligibleContentTypes) {
            std::strncpy(p.eligible_content_types[p.eligible_content_type_count++], t,
                         policy::kMaxContentTypeLen - 1);
        }
    }
    return p;
}

TEST(ContentTypeEligibilityTest, ApplicationJson_Eligible) {
    auto p = make_policy({ "application/json" });
    const char* ct = "application/json";
    EXPECT_TRUE(is_content_type_eligible(p, ct, std::strlen(ct)));
}

TEST(ContentTypeEligibilityTest, CharsetVariant_Matches) {
    auto p = make_policy({ "application/json" });
    const char* ct = "application/json; charset=utf-8";
    EXPECT_TRUE(is_content_type_eligible(p, ct, std::strlen(ct)));
}

TEST(ContentTypeEligibilityTest, TextPlain_Eligible) {
    auto p = make_policy({ "application/json", "text/plain" });
    const char* ct = "text/plain";
    EXPECT_TRUE(is_content_type_eligible(p, ct, std::strlen(ct)));
}

TEST(ContentTypeEligibilityTest, ImagePng_Rejected) {
    auto p = make_policy({ "application/json" });
    const char* ct = "image/png";
    EXPECT_FALSE(is_content_type_eligible(p, ct, std::strlen(ct)));
}

TEST(ContentTypeEligibilityTest, MissingContentType_Rejected) {
    auto p = make_policy({ "application/json" });
    EXPECT_FALSE(is_content_type_eligible(p, nullptr, 0));
    EXPECT_FALSE(is_content_type_eligible(p, "", 0));
}

TEST(ContentTypeEligibilityTest, EmptyAllowlist_AllTypesEligible) {
    policy::CompressionPolicy p{};
    p.enabled = true;
    // eligible_content_type_count == 0
    const char* ct = "image/png";
    EXPECT_TRUE(is_content_type_eligible(p, ct, std::strlen(ct)));
}

} // namespace bytetaper::compression
