// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "policy/compression_policy.h"

#include <cstddef>

namespace bytetaper::compression {

struct ContentEncodingResult {
    bool already_encoded = false;
    // Recognized encoding, or None if the header is missing or the algorithm
    // is not one of gzip/br/zstd. already_encoded may be true with encoding=None
    // when an unrecognized encoding is present.
    policy::CompressionAlgorithm encoding = policy::CompressionAlgorithm::None;
};

// Parses a Content-Encoding response header value.
// Returns already_encoded=true for any non-empty header value.
// header may be nullptr or empty — already_encoded=false in that case.
ContentEncodingResult detect_content_encoding(const char* header, std::size_t len);

} // namespace bytetaper::compression
