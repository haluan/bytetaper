// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_JSON_TRANSFORM_CONTENT_TYPE_H
#define BYTETAPER_JSON_TRANSFORM_CONTENT_TYPE_H

namespace bytetaper::json_transform {

enum class JsonResponseKind {
    EligibleJson,
    SkipUnsupported,
    InvalidInput,
};

// Detects whether a Content-Type value should be treated as JSON.
// - Returns false only for invalid API usage (for example null out_kind).
// - Null/empty/unsupported content type returns true with SkipUnsupported.
bool detect_application_json_response(const char* content_type, JsonResponseKind* out_kind);

} // namespace bytetaper::json_transform

#endif // BYTETAPER_JSON_TRANSFORM_CONTENT_TYPE_H
