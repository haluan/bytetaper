// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_L2_CACHE_ASYNC_STORE_ENQUEUE_STAGE_H
#define BYTETAPER_STAGES_L2_CACHE_ASYNC_STORE_ENQUEUE_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

// APG pipeline stage: enqueues a best-effort async L2 store on eligible responses.
// Always returns StageResult::Continue — never blocks, never fails response.
// Skips safely when: body oversized, queue full, no worker queue, or ineligible.
apg::StageOutput l2_cache_async_store_enqueue_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_L2_CACHE_ASYNC_STORE_ENQUEUE_STAGE_H
