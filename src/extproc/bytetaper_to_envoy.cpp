// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/bytetaper_to_envoy.h"

#include "extproc/reporting_headers.h"

#include <envoy/config/core/v3/base.pb.h>
#include <envoy/type/v3/http_status.pb.h>

namespace bytetaper::extproc {

static void add_header(envoy::service::ext_proc::v3::ImmediateResponse* imm, const char* key,
                       const std::string& value) {
    auto* mutation = imm->mutable_headers()->add_set_headers();
    mutation->mutable_header()->set_key(key);
    mutation->mutable_header()->set_raw_value(value);
    mutation->set_append_action(
        envoy::config::core::v3::HeaderValueOption::OVERWRITE_IF_EXISTS_OR_ADD);
}

bool map_cache_hit_to_immediate_response(
    const apg::ApgTransformContext& ctx,
    envoy::service::ext_proc::v3::ProcessingResponse* response) {

    if (!ctx.should_return_immediate_response) {
        return false;
    }

    const cache::CacheEntry& entry = ctx.cached_response;
    auto* imm = response->mutable_immediate_response();

    imm->mutable_status()->set_code(static_cast<envoy::type::v3::StatusCode>(entry.status_code));

    if (entry.body != nullptr && entry.body_len > 0) {
        imm->set_body(std::string(entry.body, entry.body_len));
    }

    if (entry.content_type[0] != '\0') {
        add_header(imm, "content-type", entry.content_type);
    }

    add_header(imm, kCachedResponseHeader, kTrueValue);

    if (ctx.cache_layer != nullptr) {
        add_header(imm, kCacheLayerHeader, ctx.cache_layer);
    }

    return true;
}

} // namespace bytetaper::extproc
