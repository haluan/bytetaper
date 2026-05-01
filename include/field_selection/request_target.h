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
bool parse_fields_query_parameter(const apg::ApgTransformContext& context,
                                  ParsedFieldsQuery* parsed_fields);

} // namespace bytetaper::field_selection

#endif // BYTETAPER_FIELD_SELECTION_REQUEST_TARGET_H
