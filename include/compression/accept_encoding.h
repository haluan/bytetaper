// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstddef>

namespace bytetaper::compression {

// Parsed result of an Accept-Encoding header value.
// q-values are accepted in input but ignored — presence only is recorded.
struct AcceptEncoding {
    bool supports_gzip = false;
    bool supports_br = false;
    bool supports_zstd = false;
    bool supports_deflate = false;
    bool supports_identity = false;
};

// Parses a comma-separated Accept-Encoding header value into AcceptEncoding.
// header may be nullptr or empty — all fields will be false in that case.
// q-values (e.g., "gzip;q=0.9") are stripped before matching; preference
// ordering is not preserved.
AcceptEncoding parse_accept_encoding(const char* header, std::size_t len);

} // namespace bytetaper::compression
