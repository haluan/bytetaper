// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/accept_encoding.h"

#include <cctype>
#include <cstring>

namespace bytetaper::compression {

namespace {

// Returns true if two null-terminated strings match case-insensitively.
static bool iequal(const char* a, const char* b) {
    while (*a && *b) {
        if (std::tolower(static_cast<unsigned char>(*a)) !=
            std::tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

// Trims leading and trailing ASCII whitespace in-place by adjusting start/len.
static void trim(const char*& s, std::size_t& n) {
    while (n > 0 && std::isspace(static_cast<unsigned char>(s[0]))) {
        ++s;
        --n;
    }
    while (n > 0 && std::isspace(static_cast<unsigned char>(s[n - 1]))) {
        --n;
    }
}

static void apply_token(AcceptEncoding& out, const char* tok, std::size_t tok_len) {
    // Strip q-value: truncate at ';'
    for (std::size_t i = 0; i < tok_len; ++i) {
        if (tok[i] == ';') {
            tok_len = i;
            break;
        }
    }
    trim(tok, tok_len);
    if (tok_len == 0)
        return;

    // Copy to null-terminated buffer for iequal comparison.
    char buf[32] = {};
    if (tok_len >= sizeof(buf))
        return; // unknown long token
    std::memcpy(buf, tok, tok_len);

    if (iequal(buf, "gzip")) {
        out.supports_gzip = true;
        return;
    }
    if (iequal(buf, "br")) {
        out.supports_br = true;
        return;
    }
    if (iequal(buf, "zstd")) {
        out.supports_zstd = true;
        return;
    }
    if (iequal(buf, "deflate")) {
        out.supports_deflate = true;
        return;
    }
    if (iequal(buf, "identity")) {
        out.supports_identity = true;
        return;
    }
    // Unknown tokens and "*" are silently ignored.
}

} // namespace

AcceptEncoding parse_accept_encoding(const char* header, std::size_t len) {
    AcceptEncoding out{};
    if (header == nullptr || len == 0)
        return out;

    const char* p = header;
    const char* end = header + len;

    while (p < end) {
        const char* comma = p;
        while (comma < end && *comma != ',')
            ++comma;
        apply_token(out, p, static_cast<std::size_t>(comma - p));
        p = (comma < end) ? comma + 1 : end;
    }
    return out;
}

} // namespace bytetaper::compression
