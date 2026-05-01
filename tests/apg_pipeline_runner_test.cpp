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

StageOutput StageA(char* execution_log, std::size_t* write_index, ApgTransformContext& context) {
    execution_log[*write_index] = 'A';
    *write_index += 1;
    context.executed_stage_count += 1;
    return StageOutput{
        .result = StageResult::Continue,
        .note = "A",
    };
}

StageOutput StageB(char* execution_log, std::size_t* write_index, ApgTransformContext& context) {
    execution_log[*write_index] = 'B';
    *write_index += 1;
    context.executed_stage_count += 1;
    return StageOutput{
        .result = StageResult::Error,
        .note = "B",
    };
}

StageOutput StageC(char* execution_log, std::size_t* write_index, ApgTransformContext& context) {
    execution_log[*write_index] = 'C';
    *write_index += 1;
    context.executed_stage_count += 1;
    return StageOutput{
        .result = StageResult::SkipRemaining,
        .note = "C",
    };
}

StageOutput StageAAdapter(ApgTransformContext& context) {
    char* const execution_log = reinterpret_cast<char*>(context.request_id);
    std::size_t* const write_index = reinterpret_cast<std::size_t*>(context.input_payload_bytes);
    return StageA(execution_log, write_index, context);
}

StageOutput StageBAdapter(ApgTransformContext& context) {
    char* const execution_log = reinterpret_cast<char*>(context.request_id);
    std::size_t* const write_index = reinterpret_cast<std::size_t*>(context.input_payload_bytes);
    return StageB(execution_log, write_index, context);
}

StageOutput StageCAdapter(ApgTransformContext& context) {
    char* const execution_log = reinterpret_cast<char*>(context.request_id);
    std::size_t* const write_index = reinterpret_cast<std::size_t*>(context.input_payload_bytes);
    return StageC(execution_log, write_index, context);
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

TEST(ApgPipelineRunnerTest, StageExecutionOrderMatchesRegistrationOrder) {
    char execution_log[4] = {'\0', '\0', '\0', '\0'};
    std::size_t write_index = 0;

    ApgTransformContext context{};
    context.request_id = reinterpret_cast<std::uint64_t>(execution_log);
    context.input_payload_bytes = reinterpret_cast<std::size_t>(&write_index);

    const ApgStage stages[] = {
        &StageAAdapter,
        &StageBAdapter,
        &StageCAdapter,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    execution_log[write_index] = '\0';

    EXPECT_EQ(context.executed_stage_count, 3u);
    EXPECT_STREQ(execution_log, "ABC");
    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(output.note, "C");
}

} // namespace bytetaper::apg
