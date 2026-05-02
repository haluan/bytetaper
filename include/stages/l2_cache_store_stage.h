// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_STAGES_L2_CACHE_STORE_STAGE_H
#define BYTETAPER_STAGES_L2_CACHE_STORE_STAGE_H

#include "apg/context.h"
#include "apg/result.h"

namespace bytetaper::stages {

// APG pipeline stage: stores eligible upstream responses into L2 (RocksDB).
// Always returns StageResult::Continue — side-effect stage.
// Stores only when: cache enabled, method GET, status 2xx, body non-empty, TTL > 0.
apg::StageOutput l2_cache_store_stage(apg::ApgTransformContext& context);

} // namespace bytetaper::stages

#endif // BYTETAPER_STAGES_L2_CACHE_STORE_STAGE_H
