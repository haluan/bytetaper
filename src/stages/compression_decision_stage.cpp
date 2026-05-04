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
        if (context.compression_metrics != nullptr) {
            metrics::record_compression_event(context.compression_metrics,
                                              metrics::CompressionMetricEvent::SkipPolicyDisabled);
        }
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

    if (context.compression_metrics != nullptr) {
        if (decision.candidate) {
            metrics::record_compression_event(context.compression_metrics,
                                              metrics::CompressionMetricEvent::Candidate);
        } else if (decision.reason != nullptr) {
            if (std::strcmp(decision.reason, "client-unsupported") == 0) {
                metrics::record_compression_event(
                    context.compression_metrics,
                    metrics::CompressionMetricEvent::SkipClientUnsupported);
            } else if (std::strcmp(decision.reason, "already-encoded") == 0) {
                metrics::record_compression_event(
                    context.compression_metrics,
                    metrics::CompressionMetricEvent::SkipAlreadyEncoded);
            } else if (std::strcmp(decision.reason, "content-type-ineligible") == 0) {
                metrics::record_compression_event(context.compression_metrics,
                                                  metrics::CompressionMetricEvent::SkipContentType);
            } else if (std::strcmp(decision.reason, "response-too-small") == 0) {
                metrics::record_compression_event(
                    context.compression_metrics,
                    metrics::CompressionMetricEvent::SkipResponseTooSmall);
            } else if (std::strcmp(decision.reason, "non-2xx-response") == 0) {
                metrics::record_compression_event(context.compression_metrics,
                                                  metrics::CompressionMetricEvent::SkipNon2xx);
            }
        }
    }

    return { apg::StageResult::Continue,
             decision.candidate ? "compression-candidate" : decision.reason };
}

} // namespace bytetaper::stages
