// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/stage.h"

#include <gtest/gtest.h>

namespace bytetaper::apg {

namespace {

StageOutput IncrementStageCount(ApgTransformContext& context) {
    context.executed_stage_count += 1;
    return StageOutput{
        .result = StageResult::Continue,
        .note = "stage advanced",
    };
}

} // namespace

TEST(ApgStageSignatureTest, AssignAndDispatchFunctionPointer) {
    ApgTransformContext context{};
    ApgStage stage = &IncrementStageCount;

    const StageOutput output = stage(context);

    EXPECT_EQ(output.result, StageResult::Continue);
    EXPECT_STREQ(output.note, "stage advanced");
    EXPECT_EQ(context.executed_stage_count, 1u);
}

} // namespace bytetaper::apg
