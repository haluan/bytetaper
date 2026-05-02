// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_L1_CACHE_LOOKUP_STAGE_H
#define BYTETAPER_STAGES_L1_CACHE_LOOKUP_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

// APG pipeline stage: checks L1 cache before upstream execution.
// On hit:  sets context.cache_hit, context.cache_layer, context.should_return_immediate_response,
//          context.cached_response; returns StageResult::SkipRemaining.
// On miss: returns StageResult::Continue.
apg::StageOutput l1_cache_lookup_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_L1_CACHE_LOOKUP_STAGE_H
