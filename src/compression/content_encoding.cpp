// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/content_encoding.h"

#include <cctype>
#include <cstring>

namespace bytetaper::compression {

namespace {

static bool iequal_n(const char* a, std::size_t alen, const char* b) {
    std::size_t blen = std::strlen(b);
    if (alen != blen)
        return false;
    for (std::size_t i = 0; i < alen; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

ContentEncodingResult detect_content_encoding(const char* header, std::size_t len) {
    ContentEncodingResult out{};
    if (header == nullptr || len == 0)
        return out;

    // Trim leading/trailing whitespace.
    while (len > 0 && std::isspace(static_cast<unsigned char>(header[0]))) {
        ++header;
        --len;
    }
    while (len > 0 && std::isspace(static_cast<unsigned char>(header[len - 1]))) {
        --len;
    }
    if (len == 0)
        return out;

    // Any non-empty Content-Encoding means the response is already encoded.
    out.already_encoded = true;

    // Match the first token (before any comma or semicolon) for the algorithm.
    std::size_t tok_len = 0;
    while (tok_len < len && header[tok_len] != ',' && header[tok_len] != ';')
        ++tok_len;

    // Trim token.
    const char* tok = header;
    while (tok_len > 0 && std::isspace(static_cast<unsigned char>(tok[0]))) {
        ++tok;
        --tok_len;
    }
    while (tok_len > 0 && std::isspace(static_cast<unsigned char>(tok[tok_len - 1]))) {
        --tok_len;
    }

    if (iequal_n(tok, tok_len, "gzip"))
        out.encoding = policy::CompressionAlgorithm::Gzip;
    else if (iequal_n(tok, tok_len, "br"))
        out.encoding = policy::CompressionAlgorithm::Brotli;
    else if (iequal_n(tok, tok_len, "zstd"))
        out.encoding = policy::CompressionAlgorithm::Zstd;
    // deflate and unknown encodings: already_encoded=true, encoding=None

    return out;
}

} // namespace bytetaper::compression
