// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_COALESCING_FOLLOWER_WAIT_STAGE_H
#define BYTETAPER_STAGES_COALESCING_FOLLOWER_WAIT_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

/**
 * @brief APG pipeline stage: Follower wait and cache check.
 *
 * If the request is a Coalescing Follower, this stage will poll the caches
 * (L1 and L2) until a hit is found or the wait window expires.
 *
 * On cache hit: sets context cache fields and returns StageResult::SkipRemaining.
 * On miss/timeout: returns StageResult::Continue.
 */
apg::StageOutput coalescing_follower_wait_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_COALESCING_FOLLOWER_WAIT_STAGE_H
