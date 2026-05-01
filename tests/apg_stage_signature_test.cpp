// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/stage.h"

#include <gtest/gtest.h>

namespace bytetaper::apg {

namespace {

bool IncrementStageCount(ApgTransformContext& context) {
    context.executed_stage_count += 1;
    return true;
}

} // namespace

TEST(ApgStageSignatureTest, AssignAndDispatchFunctionPointer) {
    ApgTransformContext context{};
    ApgStage stage = &IncrementStageCount;

    const bool stage_ok = stage(context);

    EXPECT_TRUE(stage_ok);
    EXPECT_EQ(context.executed_stage_count, 1u);
}

} // namespace bytetaper::apg
