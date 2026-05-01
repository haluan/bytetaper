// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "json_transform/content_type.h"

#include <cstddef>
#include <cstring>

namespace bytetaper::json_transform {

namespace {

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
};

const FlatJsonFieldView* find_field(const ParsedFlatJsonObject& parsed, const char* key) {
    if (key == nullptr) {
        return nullptr;
    }

    const std::size_t field_count =
        (parsed.field_count <= policy::kMaxFields) ? parsed.field_count : policy::kMaxFields;
    for (std::size_t index = 0; index < field_count; ++index) {
        if (std::strcmp(parsed.fields[index].key, key) == 0) {
            return &parsed.fields[index];
        }
    }
    return nullptr;
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

    const std::size_t selected_count = (context.selected_field_count <= policy::kMaxFields)
                                           ? context.selected_field_count
                                           : policy::kMaxFields;
    for (std::size_t selected_index = 0; selected_index < selected_count; ++selected_index) {
        const FlatJsonFieldView* field =
            find_field(parsed, context.selected_fields[selected_index]);
        if (field == nullptr) {
            continue;
        }
        if (field->value_begin > field->value_end || field->value_end > parsed.source_length) {
            return FlatJsonFilterStatus::SkipUnsupported;
        }

        if (!first_emitted) {
            writer.append_char(',');
        }
        first_emitted = false;

        writer.append_char('"');
        writer.append_cstr(field->key);
        writer.append_cstr("\":");
        writer.append_slice(parsed.source, field->value_begin, field->value_end);
    }

    writer.append_char('}');
    *output_length = writer.length;
    return writer.fits_capacity() ? FlatJsonFilterStatus::Ok : FlatJsonFilterStatus::OutputTooSmall;
}

} // namespace bytetaper::json_transform
