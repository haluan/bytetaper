// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
#define BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H

#include "apg/context.h"
#include "policy/field_filter_policy.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bytetaper::field_selection {

struct CompiledFieldSelection {
    const char* exact_paths[policy::kMaxFields];
    std::uint32_t exact_hashes[policy::kMaxFields];
    std::size_t exact_lens[policy::kMaxFields];
    std::size_t exact_count = 0;

    const char* descendant_paths[policy::kMaxFields];
    std::uint32_t descendant_hashes[policy::kMaxFields];
    std::size_t descendant_lens[policy::kMaxFields];
    std::size_t descendant_count = 0;
};

struct SelectionMatchResult {
    bool exact_selected = false;
    bool has_selected_descendant = false;
};

inline bool starts_with_path_prefix(const char* full_path, const char* selected) {
    if (full_path == nullptr || selected == nullptr) {
        return false;
    }

    std::size_t index = 0;
    while (full_path[index] != '\0' && selected[index] == full_path[index]) {
        index += 1;
    }
    return full_path[index] == '\0' && (selected[index] == '.' || selected[index] == '[');
}

inline std::uint32_t field_hash(const char* s) {
    if (s == nullptr) {
        return 0;
    }
    std::uint32_t h = 5381;
    while (unsigned char c = static_cast<unsigned char>(*s++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

inline void compile_field_selection(const apg::ApgTransformContext& context,
                                    CompiledFieldSelection* out) {
    if (out == nullptr) {
        return;
    }
    out->exact_count = 0;
    out->descendant_count = 0;
    const std::size_t n = (context.selected_field_count <= policy::kMaxFields)
                              ? context.selected_field_count
                              : policy::kMaxFields;
    for (std::size_t i = 0; i < n; ++i) {
        const char* s = context.selected_fields[i];
        if (s == nullptr || s[0] == '\0') {
            continue;
        }

        const std::uint32_t h = field_hash(s);
        std::size_t len = std::strlen(s);

        if (out->exact_count < policy::kMaxFields) {
            out->exact_paths[out->exact_count] = s;
            out->exact_hashes[out->exact_count] = h;
            out->exact_lens[out->exact_count] = len;
            ++out->exact_count;
        }
        if (out->descendant_count < policy::kMaxFields) {
            out->descendant_paths[out->descendant_count] = s;
            out->descendant_hashes[out->descendant_count] = h;
            out->descendant_lens[out->descendant_count] = len;
            ++out->descendant_count;
        }
    }
}

inline SelectionMatchResult match_selection_compiled(const CompiledFieldSelection& sel,
                                                     const char* full_path) {
    SelectionMatchResult result{};
    if (full_path == nullptr || full_path[0] == '\0') {
        return result;
    }

    const std::uint32_t path_hash = field_hash(full_path);
    std::size_t path_len = std::strlen(full_path);

    // Scan exact matches
    for (std::size_t i = 0; i < sel.exact_count; ++i) {
        if (sel.exact_hashes[i] == path_hash && sel.exact_lens[i] == path_len &&
            std::strcmp(sel.exact_paths[i], full_path) == 0) {
            result.exact_selected = true;
        }
    }

    // Scan descendant matches
    for (std::size_t i = 0; i < sel.descendant_count; ++i) {
        if (sel.descendant_lens[i] > path_len) {
            if (starts_with_path_prefix(full_path, sel.descendant_paths[i])) {
                result.has_selected_descendant = true;
            }
        }
    }
    return result;
}

struct ParsedFieldsQuery {
    char fields[policy::kMaxFields][policy::kMaxFieldNameLen] = {};
    std::size_t field_count = 0;
};

bool extract_raw_path_and_query(const char* request_target, apg::ApgTransformContext* context);
// Parses only the first `fields` query parameter from `context.raw_query`.
// Empty parameter values and empty comma tokens are ignored safely:
// - `fields=` -> zero parsed fields
// - `fields=,,` -> zero parsed fields
// - `fields=id,,name,` -> parsed fields are `id`, `name`
bool parse_fields_query_parameter(const apg::RequestQueryView& query_view,
                                  ParsedFieldsQuery* parsed_fields);
bool parse_fields_query_parameter(const apg::ApgTransformContext& context,
                                  ParsedFieldsQuery* parsed_fields);
// Clears previous selected fields, then parses and stores bounded results into context.
// Missing/empty `fields` is non-error and leaves `selected_field_count == 0`.
// `selected_field_count` is the API-intelligence count for this phase and is
// interpreted as "currently selected fields in context" before policy enforcement.
bool parse_and_store_selected_fields(apg::ApgTransformContext* context);
// Enforces policy against context.selected_fields.
// Disallowed fields are removed and allowed fields are compacted in-place
// while preserving order. If all fields are disallowed, `selected_field_count` becomes 0.
// After this call, `selected_field_count` is the post-filter API-intelligence count.
// Returns false only for invalid input (e.g. null context).
bool enforce_selected_fields_policy(apg::ApgTransformContext* context,
                                    const policy::FieldFilterPolicy& policy);

} // namespace bytetaper::field_selection

#endif // BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
