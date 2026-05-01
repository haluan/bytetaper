// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/context.h"

#include <gtest/gtest.h>

namespace bytetaper::apg {

TEST(ApgTransformContextTest, DefaultInitZerosAllFields) {
    const ApgTransformContext context{};

    EXPECT_EQ(context.request_id, 0u);
    EXPECT_EQ(context.input_payload_bytes, 0u);
    EXPECT_EQ(context.output_payload_bytes, 0u);
    EXPECT_EQ(context.executed_stage_count, 0u);
}

TEST(ApgTransformContextTest, ExplicitAssignmentReadsBackValues) {
    ApgTransformContext context{};
    context.request_id = 42u;
    context.input_payload_bytes = 128u;
    context.output_payload_bytes = 64u;
    context.executed_stage_count = 3u;

    EXPECT_EQ(context.request_id, 42u);
    EXPECT_EQ(context.input_payload_bytes, 128u);
    EXPECT_EQ(context.output_payload_bytes, 64u);
    EXPECT_EQ(context.executed_stage_count, 3u);
}

} // namespace bytetaper::apg
