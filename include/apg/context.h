// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_CONTEXT_H
#define BYTETAPER_APG_CONTEXT_H

#include "policy/field_filter_policy.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::apg {

struct ApgTransformContext {
    static constexpr std::size_t kRawPathBufferSize = 1024;
    static constexpr std::size_t kRawQueryBufferSize = 1024;

    std::uint64_t request_id = 0;
    std::size_t input_payload_bytes = 0;
    std::size_t output_payload_bytes = 0;
    std::uint32_t executed_stage_count = 0;
    char raw_path[kRawPathBufferSize] = {};
    std::size_t raw_path_length = 0;
    char raw_query[kRawQueryBufferSize] = {};
    std::size_t raw_query_length = 0;
    char selected_fields[policy::kMaxFields][policy::kMaxFieldNameLen] = {};
    // Canonical API-intelligence metric.
    // This count represents selected fields after policy filtering.
    std::size_t selected_field_count = 0;
    char* trace_buffer = nullptr;
    std::size_t trace_capacity = 0;
    std::size_t trace_length = 0;
};

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_CONTEXT_H
