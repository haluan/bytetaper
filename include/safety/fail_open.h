// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_SAFETY_FAIL_OPEN_H
#define BYTETAPER_SAFETY_FAIL_OPEN_H

#include "json_transform/content_type.h"

namespace bytetaper::safety {

enum class FailOpenReason {
    None,
    SkipUnsupported,
    InvalidInput,
    OutputTooSmall,
    InvalidJsonSafeError,
    PayloadTooLarge,
    Non2xxResponse,
    NonJsonResponse,
    UnknownError
};

struct FailOpenDecision {
    bool should_mutate;
    FailOpenReason reason;
};

FailOpenDecision evaluate_filter_safety(json_transform::FlatJsonFilterStatus status);
const char* get_fail_open_reason_string(FailOpenReason reason);

} // namespace bytetaper::safety

#endif // BYTETAPER_SAFETY_FAIL_OPEN_H
