// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_COALESCING_DECISION_STAGE_H
#define BYTETAPER_STAGES_COALESCING_DECISION_STAGE_H

#include "apg/stage.h"

namespace bytetaper::stages {

// Evaluates coalescing eligibility and registers with the in-flight registry.
apg::StageOutput coalescing_decision_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_COALESCING_DECISION_STAGE_H
