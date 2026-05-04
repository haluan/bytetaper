// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/l2_cache_async_store_enqueue_stage.h"

#include "cache/cache_key.h"
#include "runtime/worker_queue.h"

#include <cstring>

namespace bytetaper::stages {

apg::StageOutput l2_cache_async_store_enqueue_stage(apg::ApgTransformContext& context) {
    // 1. Policy presence
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    // 2. Cache enabled
    if (context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {
        return { apg::StageResult::Continue, "cache-disabled" };
    }

    // 3. GET method only
    if (context.request_method != policy::HttpMethod::Get) {
        return { apg::StageResult::Continue, "non-get" };
    }

    // 4. 2xx status only
    if (context.response_status_code < 200 || context.response_status_code >= 300) {
        return { apg::StageResult::Continue, "non-2xx" };
    }

    // 5. Body presence
    if (context.response_body == nullptr || context.response_body_len == 0) {
        return { apg::StageResult::Continue, "no-body" };
    }

    // 6. Non-zero TTL
    if (context.matched_policy->cache.ttl_seconds == 0) {
        return { apg::StageResult::Continue, "no-ttl" };
    }

    // 7. L2 and Worker pointers present
    if (context.l2_cache == nullptr) {
        return { apg::StageResult::Continue, "no-l2-cache" };
    }
    if (context.worker_queue == nullptr) {
        return { apg::StageResult::Continue, "no-worker-queue" };
    }

    // 8. Body size cap
    if (context.response_body_len > runtime::kAsyncL2MaxBodySize) {
        return { apg::StageResult::Continue, "body-too-large" };
    }

    // 9. Build cache key
    runtime::RuntimeCacheJob job{};
    job.kind = runtime::RuntimeJobKind::L2Store;

    cache::CacheKeyInput ki{};
    ki.method = context.request_method;
    ki.route_id = context.matched_policy->route_id;
    ki.path = context.raw_path;
    ki.query = context.raw_query;
    ki.selected_fields = context.selected_fields;
    ki.selected_field_count = context.selected_field_count;
    ki.policy_version = context.matched_policy->route_id;

    if (!cache::build_cache_key(ki, job.key, sizeof(job.key))) {
        return { apg::StageResult::Continue, "key-build-failed" };
    }

    // 10. Deep copy body into job buffer
    std::memcpy(job.body, context.response_body, context.response_body_len);
    job.body_len = context.response_body_len;

    // 11. Populate CacheEntry metadata
    std::memcpy(job.entry.key, job.key, sizeof(job.key));
    job.entry.status_code = context.response_status_code;
    std::memcpy(job.entry.content_type, context.response_content_type,
                sizeof(job.entry.content_type));
    job.entry.body_len = context.response_body_len;
    job.entry.created_at_epoch_ms = context.request_epoch_ms;
    job.entry.expires_at_epoch_ms =
        context.request_epoch_ms +
        static_cast<std::int64_t>(context.matched_policy->cache.ttl_seconds) * 1000;

    // 12. Enqueue
    if (!runtime::worker_queue_try_enqueue(context.worker_queue, job)) {
        return { apg::StageResult::Continue, "queue-full" };
    }

    return { apg::StageResult::Continue, "enqueued" };
}

} // namespace bytetaper::stages
