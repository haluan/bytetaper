// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "json_transform/content_type.h"

#include <cstddef>
#include <cstring>

namespace bytetaper::json_transform {

namespace {

constexpr const char* kInvalidJsonSafeErrorBody = R"({"error":"invalid_json"})";
constexpr std::size_t kMaxSelectionPathLen = 512;

struct BoundedWriter {
    char* output = nullptr;
    std::size_t capacity = 0;
    std::size_t length = 0;

    void initialize() {
        if (output != nullptr && capacity > 0) {
            output[0] = '\0';
        }
    }

    void append_char(char ch) {
        if (output != nullptr && capacity > 0 && length + 1 < capacity) {
            output[length] = ch;
        }
        length += 1;
        if (output != nullptr && capacity > 0) {
            const std::size_t terminator_index = (length < capacity) ? length : (capacity - 1);
            output[terminator_index] = '\0';
        }
    }

    void append_cstr(const char* text) {
        if (text == nullptr) {
            return;
        }
        std::size_t index = 0;
        while (text[index] != '\0') {
            append_char(text[index]);
            index += 1;
        }
    }

    void append_slice(const char* text, std::size_t begin, std::size_t end) {
        for (std::size_t index = begin; index < end; ++index) {
            append_char(text[index]);
        }
    }

    bool fits_capacity() const {
        return capacity > 0 && length + 1 <= capacity;
    }

    std::size_t checkpoint() const {
        return length;
    }

    void rollback(std::size_t checkpoint_length) {
        length = checkpoint_length;
        if (output != nullptr && capacity > 0) {
            const std::size_t terminator_index = (length < capacity) ? length : (capacity - 1);
            output[terminator_index] = '\0';
        }
    }
};

bool is_field_selected(const apg::ApgTransformContext& context, const char* key) {
    if (key == nullptr) {
        return false;
    }

    const std::size_t selected_count = (context.selected_field_count <= policy::kMaxFields)
                                           ? context.selected_field_count
                                           : policy::kMaxFields;
    for (std::size_t index = 0; index < selected_count; ++index) {
        if (std::strcmp(context.selected_fields[index], key) == 0) {
            return true;
        }
    }
    return false;
}

bool selection_contains_array_notation(const apg::ApgTransformContext& context) {
    const std::size_t selected_count = (context.selected_field_count <= policy::kMaxFields)
                                           ? context.selected_field_count
                                           : policy::kMaxFields;
    for (std::size_t index = 0; index < selected_count; ++index) {
        const char* selected = context.selected_fields[index];
        if (selected == nullptr) {
            continue;
        }
        std::size_t ch_index = 0;
        while (selected[ch_index] != '\0') {
            if (selected[ch_index] == '[' || selected[ch_index] == ']') {
                return true;
            }
            ch_index += 1;
        }
    }
    return false;
}

bool starts_with_path_prefix(const char* full_path, const char* selected) {
    if (full_path == nullptr || selected == nullptr) {
        return false;
    }

    std::size_t index = 0;
    while (full_path[index] != '\0' && selected[index] == full_path[index]) {
        index += 1;
    }
    return full_path[index] == '\0' && selected[index] == '.';
}

struct SelectionMatch {
    bool exact_selected = false;
    bool has_selected_descendant = false;
};

SelectionMatch match_selection_path(const apg::ApgTransformContext& context,
                                    const char* full_path) {
    SelectionMatch match{};
    if (full_path == nullptr || full_path[0] == '\0') {
        return match;
    }

    const std::size_t selected_count = (context.selected_field_count <= policy::kMaxFields)
                                           ? context.selected_field_count
                                           : policy::kMaxFields;
    for (std::size_t index = 0; index < selected_count; ++index) {
        const char* selected = context.selected_fields[index];
        if (selected == nullptr || selected[0] == '\0') {
            continue;
        }
        if (std::strcmp(selected, full_path) == 0) {
            match.exact_selected = true;
            continue;
        }
        if (starts_with_path_prefix(full_path, selected)) {
            match.has_selected_descendant = true;
        }
    }
    return match;
}

bool is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

void skip_whitespace(const char* body, std::size_t length, std::size_t* index) {
    while (*index < length && is_whitespace(body[*index])) {
        *index += 1;
    }
}

bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool parse_json_string_token(const char* body, std::size_t length, std::size_t* index,
                             std::size_t* token_begin, std::size_t* token_end, char* decoded_key,
                             std::size_t decoded_key_capacity, std::size_t* decoded_key_length) {
    if (*index >= length || body[*index] != '"') {
        return false;
    }

    if (token_begin != nullptr) {
        *token_begin = *index;
    }
    *index += 1;

    std::size_t written = 0;
    while (*index < length) {
        const char ch = body[*index];
        if (ch == '"') {
            *index += 1;
            if (token_end != nullptr) {
                *token_end = *index;
            }
            if (decoded_key != nullptr && decoded_key_capacity > 0) {
                const std::size_t term_index =
                    (written < decoded_key_capacity) ? written : (decoded_key_capacity - 1);
                decoded_key[term_index] = '\0';
            }
            if (decoded_key_length != nullptr) {
                *decoded_key_length = (decoded_key_capacity > 0 && written >= decoded_key_capacity)
                                          ? (decoded_key_capacity - 1)
                                          : written;
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
            if (decoded_key != nullptr && decoded_key_capacity > 0 &&
                written + 1 < decoded_key_capacity) {
                decoded_key[written] = escaped;
            }
            written += 1;
            *index += 1;
            continue;
        }
        if (decoded_key != nullptr && decoded_key_capacity > 0 &&
            written + 1 < decoded_key_capacity) {
            decoded_key[written] = ch;
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

FlatJsonFilterStatus consume_json_value_object_only(const char* body, std::size_t length,
                                                    std::size_t* index);

FlatJsonFilterStatus consume_object_object_only(const char* body, std::size_t length,
                                                std::size_t* index) {
    if (*index >= length || body[*index] != '{') {
        return FlatJsonFilterStatus::SkipUnsupported;
    }
    *index += 1;
    skip_whitespace(body, length, index);
    if (*index < length && body[*index] == '}') {
        *index += 1;
        return FlatJsonFilterStatus::Ok;
    }

    while (*index < length) {
        if (!parse_json_string_token(body, length, index, nullptr, nullptr, nullptr, 0, nullptr)) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        skip_whitespace(body, length, index);
        if (*index >= length || body[*index] != ':') {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        *index += 1;
        skip_whitespace(body, length, index);
        const FlatJsonFilterStatus value_status =
            consume_json_value_object_only(body, length, index);
        if (value_status != FlatJsonFilterStatus::Ok) {
            return value_status;
        }
        skip_whitespace(body, length, index);
        if (*index >= length) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        if (body[*index] == ',') {
            *index += 1;
            skip_whitespace(body, length, index);
            continue;
        }
        if (body[*index] == '}') {
            *index += 1;
            return FlatJsonFilterStatus::Ok;
        }
        return FlatJsonFilterStatus::SkipUnsupported;
    }
    return FlatJsonFilterStatus::SkipUnsupported;
}

FlatJsonFilterStatus consume_json_value_object_only(const char* body, std::size_t length,
                                                    std::size_t* index) {
    if (*index >= length) {
        return FlatJsonFilterStatus::SkipUnsupported;
    }

    const char ch = body[*index];
    if (ch == '{') {
        return consume_object_object_only(body, length, index);
    }
    if (ch == '[') {
        return FlatJsonFilterStatus::SkipUnsupported;
    }
    if (ch == '"') {
        if (!parse_json_string_token(body, length, index, nullptr, nullptr, nullptr, 0, nullptr)) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        return FlatJsonFilterStatus::Ok;
    }
    if (ch == 't') {
        return parse_literal_token(body, length, index, "true")
                   ? FlatJsonFilterStatus::Ok
                   : FlatJsonFilterStatus::SkipUnsupported;
    }
    if (ch == 'f') {
        return parse_literal_token(body, length, index, "false")
                   ? FlatJsonFilterStatus::Ok
                   : FlatJsonFilterStatus::SkipUnsupported;
    }
    if (ch == 'n') {
        return parse_literal_token(body, length, index, "null")
                   ? FlatJsonFilterStatus::Ok
                   : FlatJsonFilterStatus::SkipUnsupported;
    }
    if (ch == '-' || is_digit(ch)) {
        return parse_number_token(body, length, index) ? FlatJsonFilterStatus::Ok
                                                       : FlatJsonFilterStatus::SkipUnsupported;
    }
    return FlatJsonFilterStatus::SkipUnsupported;
}

bool build_full_path(const char* prefix, const char* key, char* output, std::size_t capacity) {
    if (output == nullptr || capacity == 0 || key == nullptr || key[0] == '\0') {
        return false;
    }

    std::size_t index = 0;
    if (prefix != nullptr && prefix[0] != '\0') {
        while (prefix[index] != '\0' && index + 1 < capacity) {
            output[index] = prefix[index];
            index += 1;
        }
        if (prefix[index] != '\0' || index + 1 >= capacity) {
            return false;
        }
        output[index] = '.';
        index += 1;
    }
    std::size_t key_index = 0;
    while (key[key_index] != '\0' && index + 1 < capacity) {
        output[index] = key[key_index];
        index += 1;
        key_index += 1;
    }
    if (key[key_index] != '\0') {
        return false;
    }
    output[index] = '\0';
    return true;
}

FlatJsonFilterStatus filter_nested_object_value(const char* body, std::size_t length,
                                                std::size_t* index,
                                                const apg::ApgTransformContext& context,
                                                const char* prefix, BoundedWriter* writer,
                                                std::size_t* emitted_field_count);

FlatJsonFilterStatus write_filtered_object(const char* body, std::size_t length, std::size_t* index,
                                           const apg::ApgTransformContext& context,
                                           const char* prefix, BoundedWriter* writer,
                                           std::size_t* emitted_field_count) {
    if (*index >= length || body[*index] != '{' || writer == nullptr ||
        emitted_field_count == nullptr) {
        return FlatJsonFilterStatus::SkipUnsupported;
    }
    *emitted_field_count = 0;
    writer->append_char('{');
    *index += 1;
    skip_whitespace(body, length, index);
    if (*index < length && body[*index] == '}') {
        *index += 1;
        writer->append_char('}');
        return FlatJsonFilterStatus::Ok;
    }

    bool first_emitted = true;
    while (*index < length) {
        std::size_t key_token_begin = 0;
        std::size_t key_token_end = 0;
        char key[policy::kMaxFieldNameLen] = {};
        std::size_t key_length = 0;
        if (!parse_json_string_token(body, length, index, &key_token_begin, &key_token_end, key,
                                     policy::kMaxFieldNameLen, &key_length)) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        (void) key_length;

        skip_whitespace(body, length, index);
        if (*index >= length || body[*index] != ':') {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        *index += 1;
        skip_whitespace(body, length, index);

        char full_path[kMaxSelectionPathLen] = {};
        if (!build_full_path(prefix, key, full_path, sizeof(full_path))) {
            // Path too long for selection matching; consume safely and ignore field.
            const FlatJsonFilterStatus consume_status =
                consume_json_value_object_only(body, length, index);
            if (consume_status != FlatJsonFilterStatus::Ok) {
                return consume_status;
            }
        } else {
            const SelectionMatch match = match_selection_path(context, full_path);

            const std::size_t value_start = *index;
            if (body[*index] == '{') {
                if (match.exact_selected) {
                    const FlatJsonFilterStatus consume_status =
                        consume_object_object_only(body, length, index);
                    if (consume_status != FlatJsonFilterStatus::Ok) {
                        return consume_status;
                    }
                    const std::size_t value_end = *index;
                    if (!first_emitted) {
                        writer->append_char(',');
                    }
                    first_emitted = false;
                    writer->append_slice(body, key_token_begin, key_token_end);
                    writer->append_char(':');
                    writer->append_slice(body, value_start, value_end);
                    *emitted_field_count += 1;
                } else if (match.has_selected_descendant) {
                    const std::size_t member_checkpoint = writer->checkpoint();
                    if (!first_emitted) {
                        writer->append_char(',');
                    }
                    writer->append_slice(body, key_token_begin, key_token_end);
                    writer->append_char(':');

                    std::size_t nested_emitted_count = 0;
                    const FlatJsonFilterStatus nested_status = filter_nested_object_value(
                        body, length, index, context, full_path, writer, &nested_emitted_count);
                    if (nested_status != FlatJsonFilterStatus::Ok) {
                        return nested_status;
                    }
                    if (nested_emitted_count == 0) {
                        writer->rollback(member_checkpoint);
                    } else {
                        first_emitted = false;
                        *emitted_field_count += 1;
                    }
                } else {
                    const FlatJsonFilterStatus consume_status =
                        consume_object_object_only(body, length, index);
                    if (consume_status != FlatJsonFilterStatus::Ok) {
                        return consume_status;
                    }
                }
            } else {
                const std::size_t primitive_start = *index;
                const FlatJsonFilterStatus primitive_status =
                    consume_json_value_object_only(body, length, index);
                if (primitive_status != FlatJsonFilterStatus::Ok) {
                    return primitive_status;
                }
                const std::size_t primitive_end = *index;
                if (match.exact_selected) {
                    if (!first_emitted) {
                        writer->append_char(',');
                    }
                    first_emitted = false;
                    writer->append_slice(body, key_token_begin, key_token_end);
                    writer->append_char(':');
                    writer->append_slice(body, primitive_start, primitive_end);
                    *emitted_field_count += 1;
                }
            }
        }

        skip_whitespace(body, length, index);
        if (*index >= length) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
        if (body[*index] == ',') {
            *index += 1;
            skip_whitespace(body, length, index);
            continue;
        }
        if (body[*index] == '}') {
            *index += 1;
            writer->append_char('}');
            return FlatJsonFilterStatus::Ok;
        }
        return FlatJsonFilterStatus::SkipUnsupported;
    }
    return FlatJsonFilterStatus::SkipUnsupported;
}

FlatJsonFilterStatus filter_nested_object_value(const char* body, std::size_t length,
                                                std::size_t* index,
                                                const apg::ApgTransformContext& context,
                                                const char* prefix, BoundedWriter* writer,
                                                std::size_t* emitted_field_count) {
    return write_filtered_object(body, length, index, context, prefix, writer, emitted_field_count);
}

FlatJsonFilterStatus filter_nested_json_by_selected_fields(const char* input_body,
                                                           const apg::ApgTransformContext& context,
                                                           char* output,
                                                           std::size_t output_capacity,
                                                           std::size_t* output_length) {
    if (input_body == nullptr || output == nullptr || output_length == nullptr) {
        return FlatJsonFilterStatus::InvalidInput;
    }

    *output_length = 0;
    BoundedWriter writer{ output, output_capacity, 0 };
    writer.initialize();

    if (selection_contains_array_notation(context)) {
        return FlatJsonFilterStatus::SkipUnsupported;
    }

    std::size_t length = 0;
    while (input_body[length] != '\0') {
        length += 1;
    }

    std::size_t index = 0;
    skip_whitespace(input_body, length, &index);
    if (index >= length || input_body[index] != '{') {
        return FlatJsonFilterStatus::SkipUnsupported;
    }

    std::size_t emitted_count = 0;
    const FlatJsonFilterStatus status =
        write_filtered_object(input_body, length, &index, context, "", &writer, &emitted_count);
    if (status != FlatJsonFilterStatus::Ok) {
        return status;
    }

    skip_whitespace(input_body, length, &index);
    if (index != length) {
        return FlatJsonFilterStatus::SkipUnsupported;
    }

    *output_length = writer.length;
    return writer.fits_capacity() ? FlatJsonFilterStatus::Ok : FlatJsonFilterStatus::OutputTooSmall;
}

FlatJsonFilterStatus copy_original_body(const char* input_body, char* output,
                                        std::size_t output_capacity, std::size_t* output_length) {
    if (input_body == nullptr || output == nullptr || output_length == nullptr) {
        return FlatJsonFilterStatus::InvalidInput;
    }

    *output_length = 0;
    BoundedWriter writer{ output, output_capacity, 0 };
    writer.initialize();

    std::size_t index = 0;
    while (input_body[index] != '\0') {
        writer.append_char(input_body[index]);
        index += 1;
    }

    *output_length = writer.length;
    return writer.fits_capacity() ? FlatJsonFilterStatus::Ok : FlatJsonFilterStatus::OutputTooSmall;
}

FlatJsonFilterStatus write_invalid_json_safe_error(char* output, std::size_t output_capacity,
                                                   std::size_t* output_length) {
    if (output == nullptr || output_length == nullptr) {
        return FlatJsonFilterStatus::InvalidInput;
    }

    *output_length = 0;
    BoundedWriter writer{ output, output_capacity, 0 };
    writer.initialize();

    std::size_t index = 0;
    while (kInvalidJsonSafeErrorBody[index] != '\0') {
        writer.append_char(kInvalidJsonSafeErrorBody[index]);
        index += 1;
    }

    *output_length = writer.length;
    if (!writer.fits_capacity()) {
        return FlatJsonFilterStatus::OutputTooSmall;
    }
    return FlatJsonFilterStatus::InvalidJsonSafeError;
}

} // namespace

FlatJsonFilterStatus filter_flat_json_by_selected_fields(const ParsedFlatJsonObject& parsed,
                                                         const apg::ApgTransformContext& context,
                                                         char* output, std::size_t output_capacity,
                                                         std::size_t* output_length) {
    if (output == nullptr || output_length == nullptr) {
        return FlatJsonFilterStatus::InvalidInput;
    }

    *output_length = 0;
    BoundedWriter writer{ output, output_capacity, 0 };
    writer.initialize();

    if (parsed.source == nullptr) {
        return FlatJsonFilterStatus::SkipUnsupported;
    }

    writer.append_char('{');
    bool first_emitted = true;

    const std::size_t parsed_field_count =
        (parsed.field_count <= policy::kMaxFields) ? parsed.field_count : policy::kMaxFields;
    for (std::size_t field_index = 0; field_index < parsed_field_count; ++field_index) {
        const FlatJsonFieldView& field = parsed.fields[field_index];
        if (!is_field_selected(context, field.key)) {
            continue;
        }
        if (field.value_begin > field.value_end || field.value_end > parsed.source_length) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }

        if (!first_emitted) {
            writer.append_char(',');
        }
        first_emitted = false;

        writer.append_char('"');
        writer.append_cstr(field.key);
        writer.append_cstr("\":");
        writer.append_slice(parsed.source, field.value_begin, field.value_end);
    }

    writer.append_char('}');
    *output_length = writer.length;
    return writer.fits_capacity() ? FlatJsonFilterStatus::Ok : FlatJsonFilterStatus::OutputTooSmall;
}

FlatJsonFilterStatus transform_flat_json_with_filter_toggle(const char* input_body,
                                                            const ParsedFlatJsonObject* parsed,
                                                            const apg::ApgTransformContext& context,
                                                            bool filtering_enabled, char* output,
                                                            std::size_t output_capacity,
                                                            std::size_t* output_length) {
    if (parsed == nullptr) {
        return transform_flat_json_with_filter_toggle(input_body, FlatJsonParseStatus::InvalidInput,
                                                      parsed, context, filtering_enabled, output,
                                                      output_capacity, output_length);
    }
    const FlatJsonParseStatus inferred_status = (parsed->source == nullptr)
                                                    ? FlatJsonParseStatus::SkipUnsupported
                                                    : FlatJsonParseStatus::Ok;
    return transform_flat_json_with_filter_toggle(input_body, inferred_status, parsed, context,
                                                  filtering_enabled, output, output_capacity,
                                                  output_length);
}

FlatJsonFilterStatus transform_flat_json_with_filter_toggle(
    const char* input_body, FlatJsonParseStatus parse_status, const ParsedFlatJsonObject* parsed,
    const apg::ApgTransformContext& context, bool filtering_enabled, char* output,
    std::size_t output_capacity, std::size_t* output_length) {
    if (!filtering_enabled) {
        return copy_original_body(input_body, output, output_capacity, output_length);
    }

    if (parse_status == FlatJsonParseStatus::InvalidJson) {
        return write_invalid_json_safe_error(output, output_capacity, output_length);
    }

    if (parse_status == FlatJsonParseStatus::SkipUnsupported) {
        if (parsed == nullptr || parsed->source == nullptr) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }
    }

    if (parse_status == FlatJsonParseStatus::InvalidInput) {
        return FlatJsonFilterStatus::InvalidInput;
    }

    return filter_nested_json_by_selected_fields(input_body, context, output, output_capacity,
                                                 output_length);
}

} // namespace bytetaper::json_transform
