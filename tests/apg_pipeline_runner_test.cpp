// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/pipeline.h"

#include <gtest/gtest.h>

namespace bytetaper::apg {

namespace {

char* g_execution_log = nullptr;
std::size_t* g_write_index = nullptr;

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

StageOutput ContinueSecondStage(ApgTransformContext& context) {
    context.executed_stage_count += 1;
    context.output_payload_bytes = context.input_payload_bytes / 2;
    return StageOutput{
        .result = StageResult::Continue,
        .note = "second stage continue",
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

StageOutput ContinueStageB(char* execution_log, std::size_t* write_index,
                           ApgTransformContext& context) {
    execution_log[*write_index] = 'B';
    *write_index += 1;
    context.executed_stage_count += 1;
    return StageOutput{
        .result = StageResult::Continue,
        .note = "B-continue",
    };
}

StageOutput StageAAdapter(ApgTransformContext& context) {
    return StageA(g_execution_log, g_write_index, context);
}

StageOutput StageBAdapter(ApgTransformContext& context) {
    return StageB(g_execution_log, g_write_index, context);
}

StageOutput ContinueStageBAdapter(ApgTransformContext& context) {
    return ContinueStageB(g_execution_log, g_write_index, context);
}

StageOutput StageCAdapter(ApgTransformContext& context) {
    return StageC(g_execution_log, g_write_index, context);
}

} // namespace

TEST(ApgPipelineRunnerTest, ExecutesStagesInOrderAndReturnsLastOutput) {
    ApgTransformContext context{};
    char trace[8] = { '\0' };
    context.trace_buffer = trace;
    context.trace_capacity = sizeof(trace);
    const ApgStage stages[] = {
        &FirstStage,
        &ContinueSecondStage,
        &ThirdStage,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    EXPECT_EQ(context.executed_stage_count, 3u);
    EXPECT_EQ(context.input_payload_bytes, 128u);
    EXPECT_EQ(context.output_payload_bytes, 64u);
    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(output.note, "third stage");
    EXPECT_STREQ(context.trace_buffer, "CCS");
    EXPECT_EQ(context.trace_length, 3u);
}

TEST(ApgPipelineRunnerTest, EmptyPipelineReturnsDefaultOutput) {
    ApgTransformContext context{};
    char trace[4] = { 'X', 'X', '\0', '\0' };
    context.trace_buffer = trace;
    context.trace_capacity = sizeof(trace);
    context.trace_length = 2;

    const StageOutput output = run_pipeline(nullptr, 0, context);

    EXPECT_EQ(context.executed_stage_count, 0u);
    EXPECT_EQ(output.result, StageResult::Continue);
    EXPECT_EQ(output.note, nullptr);
    EXPECT_STREQ(context.trace_buffer, "");
    EXPECT_EQ(context.trace_length, 0u);
}

TEST(ApgPipelineRunnerTest, StageExecutionOrderMatchesRegistrationOrder) {
    char execution_log[4] = { '\0', '\0', '\0', '\0' };
    std::size_t write_index = 0;
    g_execution_log = execution_log;
    g_write_index = &write_index;

    ApgTransformContext context{};

    const ApgStage stages[] = {
        &StageAAdapter,
        &ContinueStageBAdapter,
        &StageCAdapter,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    execution_log[write_index] = '\0';

    EXPECT_EQ(context.executed_stage_count, 3u);
    EXPECT_STREQ(execution_log, "ABC");
    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(output.note, "C");
}

TEST(ApgPipelineRunnerTest, StopsOnErrorAndPreservesFailingOutput) {
    char execution_log[4] = { '\0', '\0', '\0', '\0' };
    std::size_t write_index = 0;
    g_execution_log = execution_log;
    g_write_index = &write_index;

    ApgTransformContext context{};
    char trace[8] = { '\0' };
    context.trace_buffer = trace;
    context.trace_capacity = sizeof(trace);

    const ApgStage stages[] = {
        &StageAAdapter,
        &StageBAdapter,
        &StageCAdapter,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    execution_log[write_index] = '\0';

    EXPECT_EQ(context.executed_stage_count, 2u);
    EXPECT_STREQ(execution_log, "AB");
    EXPECT_EQ(output.result, StageResult::Error);
    EXPECT_STREQ(output.note, "B");
    EXPECT_STREQ(context.trace_buffer, "CE");
    EXPECT_EQ(context.trace_length, 2u);
}

TEST(ApgPipelineRunnerTest, StopsOnSkipRemainingAndPreservesReason) {
    char execution_log[4] = { '\0', '\0', '\0', '\0' };
    std::size_t write_index = 0;
    g_execution_log = execution_log;
    g_write_index = &write_index;

    ApgTransformContext context{};
    char trace[8] = { '\0' };
    context.trace_buffer = trace;
    context.trace_capacity = sizeof(trace);

    const ApgStage stages[] = {
        &StageAAdapter,
        &StageCAdapter,
        &ContinueStageBAdapter,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    execution_log[write_index] = '\0';

    EXPECT_EQ(context.executed_stage_count, 2u);
    EXPECT_STREQ(execution_log, "AC");
    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(output.note, "C");
    EXPECT_STREQ(context.trace_buffer, "CS");
    EXPECT_EQ(context.trace_length, 2u);
}

TEST(ApgPipelineRunnerTest, TraceTruncatesSafelyWhenBufferIsFull) {
    ApgTransformContext context{};
    char trace[3] = { '\0', '\0', '\0' };
    context.trace_buffer = trace;
    context.trace_capacity = sizeof(trace);

    const ApgStage stages[] = {
        &FirstStage,
        &ContinueSecondStage,
        &ThirdStage,
    };

    const StageOutput output = run_pipeline(stages, 3, context);

    EXPECT_EQ(output.result, StageResult::SkipRemaining);
    EXPECT_STREQ(context.trace_buffer, "CC");
    EXPECT_EQ(context.trace_length, 2u);
}

} // namespace bytetaper::apg
