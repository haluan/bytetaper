// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_CONTEXT_H
#define BYTETAPER_APG_CONTEXT_H

#include <cstddef>
#include <cstdint>

namespace bytetaper::apg {

struct ApgTransformContext {
    std::uint64_t request_id = 0;
    std::size_t input_payload_bytes = 0;
    std::size_t output_payload_bytes = 0;
    std::uint32_t executed_stage_count = 0;
    char* trace_buffer = nullptr;
    std::size_t trace_capacity = 0;
    std::size_t trace_length = 0;
};

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_CONTEXT_H
