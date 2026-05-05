// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_CACHE_KEY_PREPARE_STAGE_H
#define BYTETAPER_STAGES_CACHE_KEY_PREPARE_STAGE_H

#include "apg/context.h"
#include "apg/stage.h"

namespace bytetaper::stages {

/**
 * Builds the cache key once and stores it in context.cache_key.
 * Sets context.cache_eligible and context.cache_key_ready.
 *
 * This stage must run before any cache lookup or store stages.
 * It always returns Continue.
 */
apg::StageOutput cache_key_prepare_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_CACHE_KEY_PREPARE_STAGE_H
