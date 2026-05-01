// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/pipeline.h"

namespace bytetaper::apg {

StageOutput run_pipeline(const ApgStage* stages, std::size_t count, ApgTransformContext& context) {
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
        if (latest_output.result == StageResult::Error ||
            latest_output.result == StageResult::SkipRemaining) {
            return latest_output;
        }
    }

    return latest_output;
}

} // namespace bytetaper::apg
