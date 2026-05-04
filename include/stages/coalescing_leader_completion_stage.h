// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_COALESCING_LEADER_COMPLETION_STAGE_H
#define BYTETAPER_STAGES_COALESCING_LEADER_COMPLETION_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

/**
 * @brief Stage that signals the completion of a leader request.
 *
 * Executed on the response path for requests identified as leaders.
 * It updates the in-flight registry to reflect that the leader has finished,
 * allowing followers to proceed (either by cache hit or timeout fallback).
 *
 * @param context The transform context.
 * @return StageOutput indicating success or skip.
 */
apg::StageOutput coalescing_leader_completion_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_COALESCING_LEADER_COMPLETION_STAGE_H
