// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_L2_CACHE_ASYNC_LOOKUP_ENQUEUE_STAGE_H
#define BYTETAPER_STAGES_L2_CACHE_ASYNC_LOOKUP_ENQUEUE_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

// APG pipeline stage: enqueues a best-effort async L2 lookup on L1 miss.
// Always returns StageResult::Continue — never blocks, never fails request.
apg::StageOutput l2_cache_async_lookup_enqueue_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_L2_CACHE_ASYNC_LOOKUP_ENQUEUE_STAGE_H
