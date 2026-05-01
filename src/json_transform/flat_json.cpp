// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "json_transform/content_type.h"

#include <cstddef>

namespace bytetaper::json_transform {

namespace {

bool is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

void skip_whitespace(const char* body, std::size_t length, std::size_t* index) {
    while (*index < length && is_whitespace(body[*index])) {
        *index += 1;
    }
}

std::size_t bounded_string_length(const char* text) {
    std::size_t length = 0;
    while (text[length] != '\0') {
        length += 1;
    }
    return length;
}

bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool parse_json_string(const char* body, std::size_t length, std::size_t* index, char* output,
                       std::size_t output_capacity, std::size_t* output_length) {
    if (*index >= length || body[*index] != '"') {
        return false;
    }
    *index += 1;

    std::size_t written = 0;
    while (*index < length) {
        const char ch = body[*index];
        if (ch == '"') {
            *index += 1;
            if (output != nullptr && output_capacity > 0) {
                if (written >= output_capacity) {
                    output[output_capacity - 1] = '\0';
                } else {
                    output[written] = '\0';
                }
            }
            if (output_length != nullptr) {
                if (output_capacity > 0 && written >= output_capacity) {
                    *output_length = output_capacity - 1;
                } else {
                    *output_length = written;
                }
            }
            return true;
        }
        if (static_cast<unsigned char>(ch) < 0x20) {
            return false;
        }
        if (ch == '\\') {
            *index += 1;
            if (*index >= length) {
                return false;
            }
            const char escaped = body[*index];
            if (output != nullptr && output_capacity > 0 && written + 1 < output_capacity) {
                output[written] = escaped;
            }
            written += 1;
            *index += 1;
            continue;
        }

        if (output != nullptr && output_capacity > 0 && written + 1 < output_capacity) {
            output[written] = ch;
        }
        written += 1;
        *index += 1;
    }
    return false;
}

bool parse_number_token(const char* body, std::size_t length, std::size_t* index) {
    if (*index >= length) {
        return false;
    }

    if (body[*index] == '-') {
        *index += 1;
        if (*index >= length) {
            return false;
        }
    }

    if (body[*index] == '0') {
        *index += 1;
    } else if (is_digit(body[*index])) {
        while (*index < length && is_digit(body[*index])) {
            *index += 1;
        }
    } else {
        return false;
    }

    if (*index < length && body[*index] == '.') {
        *index += 1;
        if (*index >= length || !is_digit(body[*index])) {
            return false;
        }
        while (*index < length && is_digit(body[*index])) {
            *index += 1;
        }
    }

    if (*index < length && (body[*index] == 'e' || body[*index] == 'E')) {
        *index += 1;
        if (*index < length && (body[*index] == '+' || body[*index] == '-')) {
            *index += 1;
        }
        if (*index >= length || !is_digit(body[*index])) {
            return false;
        }
        while (*index < length && is_digit(body[*index])) {
            *index += 1;
        }
    }

    return true;
}

bool parse_literal_token(const char* body, std::size_t length, std::size_t* index,
                         const char* literal) {
    std::size_t local = *index;
    std::size_t literal_index = 0;
    while (literal[literal_index] != '\0') {
        if (local >= length || body[local] != literal[literal_index]) {
            return false;
        }
        local += 1;
        literal_index += 1;
    }
    *index = local;
    return true;
}

FlatJsonParseStatus parse_value_token(const char* body, std::size_t length, std::size_t* index) {
    if (*index >= length) {
        return FlatJsonParseStatus::InvalidJson;
    }

    const char ch = body[*index];
    if (ch == '{' || ch == '[') {
        return FlatJsonParseStatus::SkipUnsupported;
    }

    if (ch == '"') {
        std::size_t ignored_length = 0;
        if (!parse_json_string(body, length, index, nullptr, 0, &ignored_length)) {
            return FlatJsonParseStatus::InvalidJson;
        }
        return FlatJsonParseStatus::Ok;
    }

    if (ch == 't') {
        return parse_literal_token(body, length, index, "true") ? FlatJsonParseStatus::Ok
                                                                : FlatJsonParseStatus::InvalidJson;
    }
    if (ch == 'f') {
        return parse_literal_token(body, length, index, "false") ? FlatJsonParseStatus::Ok
                                                                 : FlatJsonParseStatus::InvalidJson;
    }
    if (ch == 'n') {
        return parse_literal_token(body, length, index, "null") ? FlatJsonParseStatus::Ok
                                                                : FlatJsonParseStatus::InvalidJson;
    }

    if (ch == '-' || is_digit(ch)) {
        return parse_number_token(body, length, index) ? FlatJsonParseStatus::Ok
                                                       : FlatJsonParseStatus::InvalidJson;
    }

    return FlatJsonParseStatus::InvalidJson;
}

} // namespace

FlatJsonParseStatus parse_flat_json_object(const char* body, JsonResponseKind response_kind,
                                           ParsedFlatJsonObject* out_object) {
    if (out_object == nullptr) {
        return FlatJsonParseStatus::InvalidInput;
    }
    *out_object = ParsedFlatJsonObject{};

    if (response_kind != JsonResponseKind::EligibleJson) {
        return FlatJsonParseStatus::SkipUnsupported;
    }
    if (body == nullptr) {
        return FlatJsonParseStatus::InvalidInput;
    }

    const std::size_t length = bounded_string_length(body);
    out_object->source = body;
    out_object->source_length = length;

    std::size_t index = 0;
    skip_whitespace(body, length, &index);
    if (index >= length) {
        return FlatJsonParseStatus::InvalidJson;
    }
    if (body[index] == '[') {
        return FlatJsonParseStatus::SkipUnsupported;
    }
    if (body[index] != '{') {
        return FlatJsonParseStatus::InvalidJson;
    }
    index += 1;

    skip_whitespace(body, length, &index);
    if (index < length && body[index] == '}') {
        index += 1;
        skip_whitespace(body, length, &index);
        return index == length ? FlatJsonParseStatus::Ok : FlatJsonParseStatus::InvalidJson;
    }

    while (index < length) {
        FlatJsonFieldView field{};
        if (!parse_json_string(body, length, &index, field.key, policy::kMaxFieldNameLen,
                               &field.key_length)) {
            return FlatJsonParseStatus::InvalidJson;
        }

        skip_whitespace(body, length, &index);
        if (index >= length || body[index] != ':') {
            return FlatJsonParseStatus::InvalidJson;
        }
        index += 1;

        skip_whitespace(body, length, &index);
        field.value_begin = index;
        const FlatJsonParseStatus value_status = parse_value_token(body, length, &index);
        if (value_status != FlatJsonParseStatus::Ok) {
            return value_status;
        }
        field.value_end = index;

        if (out_object->field_count < policy::kMaxFields) {
            out_object->fields[out_object->field_count] = field;
            out_object->field_count += 1;
        }

        skip_whitespace(body, length, &index);
        if (index >= length) {
            return FlatJsonParseStatus::InvalidJson;
        }
        if (body[index] == ',') {
            index += 1;
            skip_whitespace(body, length, &index);
            continue;
        }
        if (body[index] == '}') {
            index += 1;
            skip_whitespace(body, length, &index);
            return index == length ? FlatJsonParseStatus::Ok : FlatJsonParseStatus::InvalidJson;
        }
        return FlatJsonParseStatus::InvalidJson;
    }

    return FlatJsonParseStatus::InvalidJson;
}

} // namespace bytetaper::json_transform
