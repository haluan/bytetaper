// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "json_transform/content_type.h"

#include <cstddef>

namespace bytetaper::json_transform {

namespace {

bool is_ascii_space_or_tab(char ch) {
    return ch == ' ' || ch == '\t';
}

char ascii_lower(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

bool equals_ignore_ascii_case(const char* value, std::size_t begin, std::size_t end,
                              const char* expected) {
    std::size_t index = 0;
    for (std::size_t i = begin; i < end; ++i) {
        if (expected[index] == '\0') {
            return false;
        }
        if (ascii_lower(value[i]) != expected[index]) {
            return false;
        }
        index += 1;
    }
    return expected[index] == '\0';
}

bool has_json_structured_suffix(const char* value, std::size_t begin, std::size_t end) {
    // Matches application/*+json.
    if (end <= begin) {
        return false;
    }

    const char* expected_type = "application/";
    const std::size_t expected_type_length = 12;
    if (end - begin <= expected_type_length) {
        return false;
    }

    if (!equals_ignore_ascii_case(value, begin, begin + expected_type_length, expected_type)) {
        return false;
    }

    const std::size_t subtype_begin = begin + expected_type_length;
    if (subtype_begin >= end) {
        return false;
    }

    const char* suffix = "+json";
    const std::size_t suffix_length = 5;
    if (end - subtype_begin < suffix_length) {
        return false;
    }

    const std::size_t suffix_begin = end - suffix_length;
    if (!equals_ignore_ascii_case(value, suffix_begin, end, suffix)) {
        return false;
    }

    // Ensure subtype is non-empty before +json (e.g. application/+json is unsupported).
    return suffix_begin > subtype_begin;
}

} // namespace

bool detect_application_json_response(const char* content_type, JsonResponseKind* out_kind) {
    if (out_kind == nullptr) {
        return false;
    }
    *out_kind = JsonResponseKind::SkipUnsupported;

    if (content_type == nullptr || content_type[0] == '\0') {
        return true;
    }

    std::size_t end = 0;
    while (content_type[end] != '\0' && content_type[end] != ';') {
        end += 1;
    }

    std::size_t begin = 0;
    while (begin < end && is_ascii_space_or_tab(content_type[begin])) {
        begin += 1;
    }
    while (end > begin && is_ascii_space_or_tab(content_type[end - 1])) {
        end -= 1;
    }

    if (begin == end) {
        return true;
    }

    if (equals_ignore_ascii_case(content_type, begin, end, "application/json") ||
        has_json_structured_suffix(content_type, begin, end)) {
        *out_kind = JsonResponseKind::EligibleJson;
        return true;
    }

    return true;
}

} // namespace bytetaper::json_transform
