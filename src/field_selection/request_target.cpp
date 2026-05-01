// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "field_selection/request_target.h"

#include <cstddef>

namespace bytetaper::field_selection {

namespace {

std::size_t bounded_copy(const char* source, std::size_t source_length, char* destination,
                         std::size_t destination_capacity) {
    if (destination == nullptr || destination_capacity == 0) {
        return 0;
    }

    std::size_t copied_length = source_length;
    if (copied_length + 1 > destination_capacity) {
        copied_length = destination_capacity - 1;
    }

    for (std::size_t i = 0; i < copied_length; ++i) {
        destination[i] = source[i];
    }
    destination[copied_length] = '\0';
    return copied_length;
}

void clear_path_and_query(apg::ApgTransformContext* context) {
    context->raw_path_length =
        bounded_copy("", 0, context->raw_path, apg::ApgTransformContext::kRawPathBufferSize);
    context->raw_query_length =
        bounded_copy("", 0, context->raw_query, apg::ApgTransformContext::kRawQueryBufferSize);
}

} // namespace

bool extract_raw_path_and_query(const char* request_target, apg::ApgTransformContext* context) {
    if (context == nullptr) {
        return false;
    }

    if (request_target == nullptr || request_target[0] == '\0') {
        clear_path_and_query(context);
        return true;
    }

    std::size_t separator_index = 0;
    while (request_target[separator_index] != '\0' && request_target[separator_index] != '?') {
        separator_index += 1;
    }

    context->raw_path_length = bounded_copy(request_target, separator_index, context->raw_path,
                                            apg::ApgTransformContext::kRawPathBufferSize);

    if (request_target[separator_index] == '?') {
        const char* query_start = request_target + separator_index + 1;
        std::size_t query_length = 0;
        while (query_start[query_length] != '\0') {
            query_length += 1;
        }
        context->raw_query_length = bounded_copy(query_start, query_length, context->raw_query,
                                                 apg::ApgTransformContext::kRawQueryBufferSize);
        return true;
    }

    context->raw_query_length =
        bounded_copy("", 0, context->raw_query, apg::ApgTransformContext::kRawQueryBufferSize);
    return true;
}

} // namespace bytetaper::field_selection
