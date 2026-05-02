// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_L1_CACHE_STORE_STAGE_H
#define BYTETAPER_STAGES_L1_CACHE_STORE_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

// APG pipeline stage: stores eligible upstream responses into L1 cache.
// Always returns StageResult::Continue — this is a side-effect stage.
// Stores only when: cache enabled, method GET, status 2xx, body non-empty,
// TTL > 0, and cache key builds successfully.
apg::StageOutput l1_cache_store_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_L1_CACHE_STORE_STAGE_H
