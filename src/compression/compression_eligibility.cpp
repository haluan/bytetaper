// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/compression_eligibility.h"

#include <cctype>
#include <cstring>

namespace bytetaper::compression {

namespace {

// Returns true if a and b are equal case-insensitively (a is not null-terminated,
// length alen; b is null-terminated).
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

// Returns the base MIME type length (everything before the first ';'),
// with trailing whitespace stripped.
static std::size_t base_type_len(const char* ct, std::size_t len) {
    std::size_t n = 0;
    while (n < len && ct[n] != ';')
        ++n;
    while (n > 0 && std::isspace(static_cast<unsigned char>(ct[n - 1])))
        --n;
    return n;
}

} // namespace

bool is_content_type_eligible(const policy::CompressionPolicy& policy, const char* content_type,
                              std::size_t len) {
    if (content_type == nullptr || len == 0)
        return false;

    // Strip leading whitespace.
    while (len > 0 && std::isspace(static_cast<unsigned char>(content_type[0]))) {
        ++content_type;
        --len;
    }
    if (len == 0)
        return false;

    const std::size_t base_len = base_type_len(content_type, len);
    if (base_len == 0)
        return false;

    // Empty allowlist = all content types eligible.
    if (policy.eligible_content_type_count == 0)
        return true;

    for (std::size_t i = 0; i < policy.eligible_content_type_count; ++i) {
        if (iequal_n(content_type, base_len, policy.eligible_content_types[i])) {
            return true;
        }
    }
    return false;
}

} // namespace bytetaper::compression
