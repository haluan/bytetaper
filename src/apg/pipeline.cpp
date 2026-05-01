// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/pipeline.h"

namespace bytetaper::apg {

StageOutput run_pipeline(const ApgStage* stages, std::size_t count, ApgTransformContext& context) {
    context.trace_length = 0;
    if (context.trace_buffer != nullptr && context.trace_capacity > 0) {
        context.trace_buffer[0] = '\0';
    }

    const auto append_trace = [&context](char token) {
        if (context.trace_buffer == nullptr || context.trace_capacity == 0) {
            return;
        }

        if (context.trace_length + 1 < context.trace_capacity) {
            context.trace_buffer[context.trace_length] = token;
            context.trace_length += 1;
            context.trace_buffer[context.trace_length] = '\0';
        }
    };

    if (count == 0) {
        return StageOutput{
            .result = StageResult::Continue,
            .note = nullptr,
        };
    }

    StageOutput latest_output{
        .result = StageResult::Continue,
        .note = nullptr,
    };

    for (std::size_t index = 0; index < count; ++index) {
        latest_output = stages[index](context);
        if (latest_output.result == StageResult::Continue) {
            append_trace('C');
        } else if (latest_output.result == StageResult::Error) {
            append_trace('E');
        } else if (latest_output.result == StageResult::SkipRemaining) {
            append_trace('S');
        }

        if (latest_output.result == StageResult::Error ||
            latest_output.result == StageResult::SkipRemaining) {
            return latest_output;
        }
    }

    return latest_output;
}

} // namespace bytetaper::apg
