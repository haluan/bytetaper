// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/pipeline.h"

#include <gtest/gtest.h>

namespace bytetaper::apg {

namespace {

StageOutput FirstStage(ApgTransformContext& context) {
    context.executed_stage_count += 1;
    context.input_payload_bytes = 128;
    return StageOutput{
        .result = StageResult::Continue,
        .note = "first stage",
    };
}

StageOutput SecondStage(ApgTransformContext& context) {
    context.executed_stage_count += 1;
    context.output_payload_bytes = context.input_payload_bytes / 2;
    return StageOutput{
        .result = StageResult::Error,
        .note = "second stage",
    };
}

StageOutput ThirdStage(ApgTransformContext& context) {
    context.executed_stage_count += 1;
    return StageOutput{
        .result = StageResult::SkipRemaining,
        .note = "third stage",
    };
}

} // namespace

TEST(ApgPipelineRunnerTest, ExecutesStagesInOrderAndReturnsLastOutput) {
    ApgTransformContext context{};
    const ApgStage stages[] = {
        &FirstStage,
        &SecondStage,
        &ThirdStage,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    EXPECT_EQ(context.executed_stage_count, 3u);
    EXPECT_EQ(context.input_payload_bytes, 128u);
    EXPECT_EQ(context.output_payload_bytes, 64u);
    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(output.note, "third stage");
}

TEST(ApgPipelineRunnerTest, EmptyPipelineReturnsDefaultOutput) {
    ApgTransformContext context{};

    const StageOutput output = run_pipeline(nullptr, 0, context);

    EXPECT_EQ(context.executed_stage_count, 0u);
    EXPECT_EQ(output.result, StageResult::Continue);
    EXPECT_EQ(output.note, nullptr);
}

} // namespace bytetaper::apg
