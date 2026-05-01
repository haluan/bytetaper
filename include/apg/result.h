// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_RESULT_H
#define BYTETAPER_APG_RESULT_H

namespace bytetaper::apg {

enum class StageResult {
    Continue,
    Error,
    SkipRemaining,
};

struct StageOutput {
    StageResult result = StageResult::Continue;
    const char* note = nullptr;
};

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_RESULT_H
