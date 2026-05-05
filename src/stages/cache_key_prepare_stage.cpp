// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/cache_key_prepare_stage.h"

#include "cache/cache_key.h"
#include "policy/cache_policy.h"

namespace bytetaper::stages {

apg::StageOutput cache_key_prepare_stage(apg::ApgTransformContext& context) {
    // 1. Reset state
    context.cache_eligible = false;
    context.cache_key_ready = false;

    // 2. Pre-flight checks
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    if (context.matched_policy->cache.behavior != policy::CacheBehavior::Store) {
        return { apg::StageResult::Continue, "cache-disabled" };
    }

    if (context.request_method != policy::HttpMethod::Get) {
        return { apg::StageResult::Continue, "non-get" };
    }

    // 3. Mark as eligible
    context.cache_eligible = true;

    // 4. Build key
    cache::CacheKeyInput ki{};
    ki.method = context.request_method;
    ki.route_id = context.matched_policy->route_id;
    ki.path = context.raw_path;
    ki.query = context.raw_query;
    ki.selected_fields = context.selected_fields;
    ki.selected_field_count = context.selected_field_count;
    ki.policy_version = context.matched_policy->route_id;

    if (cache::build_cache_key(ki, context.cache_key, sizeof(context.cache_key))) {
        context.cache_key_ready = true;
        return { apg::StageResult::Continue, "ready" };
    }

    return { apg::StageResult::Continue, "key-build-failed" };
}

} // namespace bytetaper::stages
