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

void clear_selected_fields(apg::ApgTransformContext* context) {
    if (context == nullptr) {
        return;
    }
    context->selected_field_count = 0;
    for (std::size_t i = 0; i < policy::kMaxFields; ++i) {
        context->selected_fields[i][0] = '\0';
    }
}

std::size_t bounded_query_length(const apg::ApgTransformContext& context) {
    std::size_t length = 0;
    while (length < context.raw_query_length &&
           length < apg::ApgTransformContext::kRawQueryBufferSize &&
           context.raw_query[length] != '\0') {
        length += 1;
    }
    return length;
}

bool query_key_is_fields(const char* query, std::size_t start, std::size_t end) {
    if (end < start) {
        return false;
    }
    const std::size_t key_length = end - start;
    if (key_length != 6) {
        return false;
    }
    return query[start + 0] == 'f' && query[start + 1] == 'i' && query[start + 2] == 'e' &&
           query[start + 3] == 'l' && query[start + 4] == 'd' && query[start + 5] == 's';
}

void parse_fields_value(const char* value, std::size_t value_length,
                        ParsedFieldsQuery* parsed_fields) {
    std::size_t token_start = 0;
    while (token_start <= value_length) {
        std::size_t token_end = token_start;
        while (token_end < value_length && value[token_end] != ',') {
            token_end += 1;
        }

        if (token_end > token_start) {
            if (parsed_fields->field_count >= policy::kMaxFields) {
                return;
            }

            const std::size_t destination_index = parsed_fields->field_count;
            const std::size_t token_length = token_end - token_start;
            const std::size_t copied_length =
                bounded_copy(value + token_start, token_length,
                             parsed_fields->fields[destination_index], policy::kMaxFieldNameLen);
            if (copied_length > 0) {
                parsed_fields->field_count += 1;
            }
        }

        if (token_end >= value_length) {
            return;
        }
        token_start = token_end + 1;
    }
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

bool parse_fields_query_parameter(const apg::ApgTransformContext& context,
                                  ParsedFieldsQuery* parsed_fields) {
    if (parsed_fields == nullptr) {
        return false;
    }

    *parsed_fields = ParsedFieldsQuery{};
    const std::size_t query_length = bounded_query_length(context);
    const char* query = context.raw_query;
    std::size_t segment_start = 0;

    while (segment_start <= query_length) {
        std::size_t segment_end = segment_start;
        while (segment_end < query_length && query[segment_end] != '&') {
            segment_end += 1;
        }

        std::size_t separator_index = segment_start;
        while (separator_index < segment_end && query[separator_index] != '=') {
            separator_index += 1;
        }

        if (query_key_is_fields(query, segment_start, separator_index)) {
            if (separator_index == segment_end) {
                return true;
            }
            const char* value = query + separator_index + 1;
            const std::size_t value_length = segment_end - (separator_index + 1);
            parse_fields_value(value, value_length, parsed_fields);
            return true;
        }

        if (segment_end >= query_length) {
            return true;
        }
        segment_start = segment_end + 1;
    }

    return true;
}

bool parse_and_store_selected_fields(apg::ApgTransformContext* context) {
    if (context == nullptr) {
        return false;
    }

    clear_selected_fields(context);
    ParsedFieldsQuery parsed_fields{};
    if (!parse_fields_query_parameter(*context, &parsed_fields)) {
        return false;
    }

    context->selected_field_count = parsed_fields.field_count;
    for (std::size_t i = 0; i < parsed_fields.field_count; ++i) {
        const std::size_t token_length =
            bounded_copy(parsed_fields.fields[i], policy::kMaxFieldNameLen - 1,
                         context->selected_fields[i], policy::kMaxFieldNameLen);
        (void) token_length;
    }
    return true;
}

} // namespace bytetaper::field_selection
