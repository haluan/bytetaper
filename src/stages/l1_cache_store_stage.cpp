// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/l1_cache_store_stage.h"

#include "cache/cache_entry.h"
#include "cache/cache_key.h"
#include "cache/l1_cache.h"
#include "policy/cache_policy.h"

#include <cstring>

namespace bytetaper::stages {

apg::StageOutput l1_cache_store_stage(apg::ApgTransformContext& context) {
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    if (context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {
        return { apg::StageResult::Continue, "cache-disabled" };
    }

    if (context.request_method != policy::HttpMethod::Get) {
        return { apg::StageResult::Continue, "non-get" };
    }

    if (context.response_status_code < 200 || context.response_status_code >= 300) {
        return { apg::StageResult::Continue, "non-2xx" };
    }

    if (context.response_body == nullptr || context.response_body_len == 0) {
        return { apg::StageResult::Continue, "no-body" };
    }

    if (context.matched_policy->cache.ttl_seconds == 0) {
        return { apg::StageResult::Continue, "no-ttl" };
    }

    if (context.l1_cache == nullptr) {
        return { apg::StageResult::Continue, "no-l1-cache" };
    }

    // Build cache key
    cache::CacheKeyInput ki{};
    ki.method = context.request_method;
    ki.route_id = context.matched_policy->route_id;
    ki.path = context.raw_path;
    ki.query = context.raw_query;
    ki.selected_fields = context.selected_fields;
    ki.selected_field_count = context.selected_field_count;
    ki.policy_version = context.matched_policy->route_id;

    char key_buf[cache::kCacheKeyMaxLen] = {};
    if (!cache::build_cache_key(ki, key_buf, sizeof(key_buf))) {
        return { apg::StageResult::Continue, "key-build-failed" };
    }

    // Build CacheEntry and store
    cache::CacheEntry entry{};
    std::strncpy(entry.key, key_buf, cache::kCacheKeyMaxLen - 1);
    entry.status_code = context.response_status_code;
    std::strncpy(entry.content_type, context.response_content_type,
                 cache::kCacheContentTypeMaxLen - 1);
    entry.body = context.response_body;
    entry.body_len = context.response_body_len;
    entry.created_at_epoch_ms = context.request_epoch_ms;
    entry.expires_at_epoch_ms =
        (context.request_epoch_ms > 0)
            ? context.request_epoch_ms +
                  static_cast<std::int64_t>(context.matched_policy->cache.ttl_seconds) * 1000
            : 0;

    cache::l1_put(context.l1_cache, entry);
    return { apg::StageResult::Continue, "stored" };
}

} // namespace bytetaper::stages
