// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
#define BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H

#include "apg/context.h"
#include "policy/field_filter_policy.h"

#include <cstddef>

namespace bytetaper::field_selection {

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
bool parse_fields_query_parameter(const apg::ApgTransformContext& context,
                                  ParsedFieldsQuery* parsed_fields);
// Clears previous selected fields, then parses and stores bounded results into context.
// Missing/empty `fields` is non-error and leaves `selected_field_count == 0`.
bool parse_and_store_selected_fields(apg::ApgTransformContext* context);

} // namespace bytetaper::field_selection

#endif // BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
