// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "safety/fail_open.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::safety {

TEST(FailOpenTest, EvaluateFilterSafety) {
    // Ok should allow mutation
    {
        auto decision = evaluate_filter_safety(json_transform::FlatJsonFilterStatus::Ok);
        EXPECT_TRUE(decision.should_mutate);
        EXPECT_EQ(decision.reason, FailOpenReason::None);
    }

    // SkipUnsupported should not mutate
    {
        auto decision =
            evaluate_filter_safety(json_transform::FlatJsonFilterStatus::SkipUnsupported);
        EXPECT_FALSE(decision.should_mutate);
        EXPECT_EQ(decision.reason, FailOpenReason::SkipUnsupported);
    }

    // InvalidInput should not mutate
    {
        auto decision = evaluate_filter_safety(json_transform::FlatJsonFilterStatus::InvalidInput);
        EXPECT_FALSE(decision.should_mutate);
        EXPECT_EQ(decision.reason, FailOpenReason::InvalidInput);
    }

    // OutputTooSmall should not mutate
    {
        auto decision =
            evaluate_filter_safety(json_transform::FlatJsonFilterStatus::OutputTooSmall);
        EXPECT_FALSE(decision.should_mutate);
        EXPECT_EQ(decision.reason, FailOpenReason::OutputTooSmall);
    }

    // InvalidJsonSafeError should not mutate
    {
        auto decision =
            evaluate_filter_safety(json_transform::FlatJsonFilterStatus::InvalidJsonSafeError);
        EXPECT_FALSE(decision.should_mutate);
        EXPECT_EQ(decision.reason, FailOpenReason::InvalidJsonSafeError);
    }

    // Timeout should not mutate
    {
        auto decision = evaluate_filter_safety(json_transform::FlatJsonFilterStatus::Timeout);
        EXPECT_FALSE(decision.should_mutate);
        EXPECT_EQ(decision.reason, FailOpenReason::Timeout);
    }
}

TEST(FailOpenTest, ReasonStrings) {
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::None), "none");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::SkipUnsupported), "skip_unsupported");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::InvalidInput), "invalid_input");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::OutputTooSmall), "output_too_small");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::InvalidJsonSafeError),
                 "invalid_json_safe_error");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::PayloadTooLarge), "payload_too_large");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::Non2xxResponse), "non_2xx_response");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::NonJsonResponse), "non_json_response");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::PolicyNotFound), "policy_not_found");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::Timeout), "timeout");
    EXPECT_STREQ(get_fail_open_reason_string(FailOpenReason::UnknownError), "unknown_error");
}

} // namespace bytetaper::safety
