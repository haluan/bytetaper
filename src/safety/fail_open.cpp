// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "safety/fail_open.h"

namespace bytetaper::safety {

FailOpenDecision evaluate_filter_safety(json_transform::FlatJsonFilterStatus status) {
    switch (status) {
    case json_transform::FlatJsonFilterStatus::Ok:
        return { true, FailOpenReason::None };
    case json_transform::FlatJsonFilterStatus::SkipUnsupported:
        return { false, FailOpenReason::SkipUnsupported };
    case json_transform::FlatJsonFilterStatus::InvalidInput:
        return { false, FailOpenReason::InvalidInput };
    case json_transform::FlatJsonFilterStatus::OutputTooSmall:
        return { false, FailOpenReason::OutputTooSmall };
    case json_transform::FlatJsonFilterStatus::InvalidJsonSafeError:
        return { false, FailOpenReason::InvalidJsonSafeError };
    case json_transform::FlatJsonFilterStatus::Timeout:
        return { false, FailOpenReason::Timeout };
    default:
        return { false, FailOpenReason::UnknownError };
    }
}

const char* get_fail_open_reason_string(FailOpenReason reason) {
    switch (reason) {
    case FailOpenReason::None:
        return "none";
    case FailOpenReason::SkipUnsupported:
        return "skip_unsupported";
    case FailOpenReason::InvalidInput:
        return "invalid_input";
    case FailOpenReason::OutputTooSmall:
        return "output_too_small";
    case FailOpenReason::InvalidJsonSafeError:
        return "invalid_json_safe_error";
    case FailOpenReason::PayloadTooLarge:
        return "payload_too_large";
    case FailOpenReason::Non2xxResponse:
        return "non_2xx_response";
    case FailOpenReason::NonJsonResponse:
        return "non_json_response";
    case FailOpenReason::PolicyNotFound:
        return "policy_not_found";
    case FailOpenReason::Timeout:
        return "timeout";
    case FailOpenReason::UnknownError:
        return "unknown_error";
    default:
        return "unknown";
    }
}

} // namespace bytetaper::safety
