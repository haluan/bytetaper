// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/result.h"

#include <gtest/gtest.h>

namespace bytetaper::apg {

TEST(ApgStageOutputTest, DefaultOutputIsContinueWithNullNote) {
    const StageOutput output{};

    EXPECT_EQ(output.result, StageResult::Continue);
    EXPECT_EQ(output.note, nullptr);
}

TEST(ApgStageOutputTest, ExplicitOutputAssignmentIsPreserved) {
    StageOutput output{};
    output.result = StageResult::SkipRemaining;
    output.note = "skip downstream stages";

    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(output.note, "skip downstream stages");
}

TEST(ApgStageResultTest, ErrorResultCanBeRepresented) {
    const StageOutput output{
        .result = StageResult::Error,
        .note = "stage failure",
    };

    EXPECT_EQ(output.result, StageResult::Error);
    EXPECT_STREQ(output.note, "stage failure");
}

} // namespace bytetaper::apg
