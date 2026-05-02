// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/l1_cache_lookup_stage.h"

#include "cache/cache_key.h"
#include "policy/cache_policy.h"

#include <cstring>

namespace bytetaper::stages {

apg::StageOutput l1_cache_lookup_stage(apg::ApgTransformContext& context) {
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    if (context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {
        return { apg::StageResult::Continue, "cache-disabled" };
    }

    // Build cache key
    cache::CacheKeyInput ki{};
    ki.method = context.request_method;
    ki.route_id = context.matched_policy->route_id;
    ki.path = context.raw_path;
    ki.query = context.raw_query;
    ki.selected_fields = context.selected_fields;
    ki.selected_field_count = context.selected_field_count;
    ki.policy_version = context.matched_policy->route_id; // Using route_id as version for now

    char key_buf[cache::kCacheKeyMaxLen] = {};
    if (!cache::build_cache_key(ki, key_buf, sizeof(key_buf))) {
        return { apg::StageResult::Continue, "key-build-failed" };
    }

    // L1 lookup
    if (context.l1_cache == nullptr) {
        return { apg::StageResult::Continue, "no-l1-cache" };
    }

    cache::CacheEntry hit{};
    if (!cache::l1_get(context.l1_cache, key_buf, context.request_epoch_ms, &hit)) {
        return { apg::StageResult::Continue, "l1-miss" };
    }

    // Hit - populate outputs
    context.cache_hit = true;
    context.cache_layer = "L1";
    context.should_return_immediate_response = true;
    context.cached_response = hit;

    return { apg::StageResult::SkipRemaining, "l1-hit" };
}

} // namespace bytetaper::stages
