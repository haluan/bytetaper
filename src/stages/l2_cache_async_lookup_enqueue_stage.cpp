// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/l2_cache_async_lookup_enqueue_stage.h"

#include "cache/cache_key.h"
#include "runtime/worker_queue.h"

#include <cstring>

namespace bytetaper::stages {

apg::StageOutput l2_cache_async_lookup_enqueue_stage(apg::ApgTransformContext& context) {
    // 1. Skip if L1 already hit
    if (context.cache_hit) {
        return apg::StageOutput{ apg::StageResult::Continue, "l1-hit-skip" };
    }

    // 2. Pre-flight checks
    if (context.matched_policy == nullptr) {
        return apg::StageOutput{ apg::StageResult::Continue, "no-policy" };
    }
    if (context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {
        return apg::StageOutput{ apg::StageResult::Continue, "cache-disabled" };
    }
    if (context.l2_cache == nullptr) {
        return apg::StageOutput{ apg::StageResult::Continue, "no-l2-cache" };
    }
    if (context.worker_queue == nullptr) {
        return apg::StageOutput{ apg::StageResult::Continue, "no-worker-queue" };
    }

    // 3. Build key
    cache::CacheKeyInput ki{};
    ki.method = context.request_method;
    ki.route_id = context.matched_policy->route_id;
    ki.path = context.raw_path;
    ki.policy_version = context.matched_policy->route_id;
    ki.selected_field_count = context.selected_field_count;
    ki.selected_fields = context.selected_fields;

    char key[cache::kCacheKeyMaxLen];
    if (!cache::build_cache_key(ki, key, sizeof(key))) {
        return apg::StageOutput{ apg::StageResult::Continue, "key-build-failed" };
    }
    // 4. Enqueue
    runtime::RuntimeCacheJob job{};
    job.kind = runtime::RuntimeJobKind::L2Lookup;
    std::strncpy(job.entry.key, key, cache::kCacheKeyMaxLen - 1);
    job.entry.key[cache::kCacheKeyMaxLen - 1] = '\0';
    job.body_len = 0; // Worker will populate this during l2_get

    if (!runtime::worker_queue_try_enqueue(context.worker_queue, job)) {
        // Enqueue might fail due to queue full OR already pending (internal to
        // worker_queue_try_enqueue). The worker_queue_try_enqueue function handles the metrics and
        // the internal pending state.
        return apg::StageOutput{ apg::StageResult::Continue, "enqueue-failed" };
    }

    metrics::record_runtime_event(context.runtime_metrics,
                                  metrics::RuntimeMetricEvent::L2LookupEnqueued);
    return apg::StageOutput{ apg::StageResult::Continue, "enqueued" };
}

} // namespace bytetaper::stages
