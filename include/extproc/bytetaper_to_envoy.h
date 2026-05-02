// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "apg/context.h"

#include <envoy/service/ext_proc/v3/external_processor.pb.h>

namespace bytetaper::extproc {

// Fills `response` with an ImmediateResponse built from ctx.cached_response.
// Returns true if ctx.should_return_immediate_response is set; false otherwise.
// Safe to call unconditionally — caller skips upstream processing on true.
bool map_cache_hit_to_immediate_response(
    const apg::ApgTransformContext& ctx,
    envoy::service::ext_proc::v3::ProcessingResponse* response);

} // namespace bytetaper::extproc
