// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_L2_CACHE_LOOKUP_STAGE_H
#define BYTETAPER_STAGES_L2_CACHE_LOOKUP_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

// APG pipeline stage: checks L2 (RocksDB) cache after an L1 miss.
// On hit:  sets context.cache_hit, context.cache_layer="L2",
//          context.should_return_immediate_response, context.cached_response;
//          returns StageResult::SkipRemaining.
// On miss: returns StageResult::Continue.
// Expired entries are not served.
apg::StageOutput l2_cache_lookup_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_L2_CACHE_LOOKUP_STAGE_H
