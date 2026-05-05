// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/default_pipelines.h"

#include "stages/coalescing_decision_stage.h"
#include "stages/coalescing_follower_wait_stage.h"
#include "stages/coalescing_leader_completion_stage.h"
#include "stages/l1_cache_lookup_stage.h"
#include "stages/l1_cache_store_stage.h"
#include "stages/l2_cache_async_lookup_enqueue_stage.h"
#include "stages/l2_cache_async_store_enqueue_stage.h"
#include "stages/pagination_request_mutation_stage.h"

#include <iterator>

namespace bytetaper::extproc {

/**
 * HOT-PATH: request-header pipeline.
 *
 * This pipeline runs synchronously on the Envoy ExtProc request thread.
 * It must NOT contain any stages that perform synchronous disk I/O.
 *
 * See docs/runtime/RUNTIME_BOUNDARIES.md for the enforcement contract.
 */
const apg::ApgStage kLookupStages[] = {
    stages::coalescing_decision_stage,
    stages::coalescing_follower_wait_stage,
    stages::l1_cache_lookup_stage,
    stages::l2_cache_async_lookup_enqueue_stage,
    stages::pagination_request_mutation_stage,
};
const std::size_t kLookupStageCount = std::size(kLookupStages);

/**
 * HOT-PATH: response-body pipeline.
 *
 * This pipeline runs synchronously on the Envoy ExtProc response thread.
 * L1 store is synchronous and fast. L2 store is async-enqueued.
 * Coalescing leader completion runs after L1 store.
 *
 * See docs/runtime/RUNTIME_BOUNDARIES.md for the enforcement contract.
 */
const apg::ApgStage kStoreStages[] = {
    stages::l1_cache_store_stage,
    stages::l2_cache_async_store_enqueue_stage,
    stages::coalescing_leader_completion_stage,
};
const std::size_t kStoreStageCount = std::size(kStoreStages);

} // namespace bytetaper::extproc
