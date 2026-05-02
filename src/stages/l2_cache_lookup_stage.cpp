// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/l2_cache_lookup_stage.h"

#include "cache/cache_key.h"
#include "cache/l2_disk_cache.h"
#include "policy/cache_policy.h"

namespace bytetaper::stages {

apg::StageOutput l2_cache_lookup_stage(apg::ApgTransformContext& context) {
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    if (context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {
        return { apg::StageResult::Continue, "cache-disabled" };
    }

    if (context.l2_cache == nullptr) {
        return { apg::StageResult::Continue, "no-l2-cache" };
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

    // L2 lookup
    cache::CacheEntry hit{};
    if (!cache::l2_get(context.l2_cache, key_buf, context.request_epoch_ms, &hit,
                       context.l2_body_buf, apg::ApgTransformContext::kL2BodyBufSize)) {
        return { apg::StageResult::Continue, "l2-miss" };
    }

    // Hit - populate outputs
    if (context.l1_cache != nullptr) {
        cache::l1_put(context.l1_cache, hit);
    }

    context.cache_hit = true;
    context.cache_layer = "L2";
    context.should_return_immediate_response = true;
    context.cached_response = hit;

    return { apg::StageResult::SkipRemaining, "l2-hit" };
}

} // namespace bytetaper::stages
