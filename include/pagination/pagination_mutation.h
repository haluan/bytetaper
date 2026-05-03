// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "pagination/pagination_decision.h"

#include <cstddef>

namespace bytetaper::pagination {

struct PaginationMutationResult {
    bool mutated = false;
    std::size_t written = 0; // bytes written to out_buf (excluding NUL)
};

// Writes the mutated path+query string into out_buf.
// If decision.should_apply_default_limit is false, copies path+query unchanged.
// Returns mutated=false and written=0 on buffer overflow.
PaginationMutationResult apply_pagination_mutation(const char* raw_path, std::size_t raw_path_len,
                                                   const char* raw_query, std::size_t raw_query_len,
                                                   const PaginationDecision& decision,
                                                   const char* limit_param, char* out_buf,
                                                   std::size_t out_buf_size);

} // namespace bytetaper::pagination
