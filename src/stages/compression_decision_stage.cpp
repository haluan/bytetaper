// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "stages/compression_decision_stage.h"

#include "compression/compression_decision.h"

#include <cstring>

namespace bytetaper::stages {

apg::StageOutput compression_decision_stage(apg::ApgTransformContext& context) {
    if (context.matched_policy == nullptr) {
        return { apg::StageResult::Continue, "no-policy" };
    }

    const auto& comp = context.matched_policy->compression;
    if (!comp.enabled) {
        return { apg::StageResult::Continue, "compression-disabled" };
    }

    compression::CompressionDecisionInput input{};
    input.compression_policy = &comp;
    input.client_encoding = context.client_accept_encoding;
    input.response_encoding = context.response_content_encoding;
    input.status_code = context.response_status_code;
    input.content_type = context.response_content_type;
    input.content_type_len = std::strlen(context.response_content_type);
    input.body_len = context.response_body_len;
    input.body_size_known = context.response_body_len > 0;

    const auto decision = compression::make_compression_decision(input);

    context.compression_decision.evaluated = true;
    context.compression_decision.candidate = decision.candidate;
    context.compression_decision.reason = decision.reason;
    context.compression_decision.algorithm_hint = decision.selected_algorithm_hint;

    return { apg::StageResult::Continue,
             decision.candidate ? "compression-candidate" : decision.reason };
}

} // namespace bytetaper::stages
