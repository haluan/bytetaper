// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstddef>
#include <cstdint>

namespace bytetaper::policy {

enum class CompressionAlgorithm : std::uint8_t {
    None = 0,
    Gzip,
    Brotli,
    Zstd,
};

enum class AlreadyEncodedBehavior : std::uint8_t {
    Skip = 0,    // do not signal compression if response is already encoded
    Passthrough, // pass through the already-encoded response unchanged
};

static constexpr std::size_t kMaxCompressionAlgorithms = 4;
static constexpr std::size_t kMaxEligibleContentTypes = 8;
static constexpr std::size_t kMaxContentTypeLen = 64;

struct CompressionPolicy {
    bool enabled = false;
    std::uint32_t min_size_bytes = 0; // 0 = no minimum threshold

    char eligible_content_types[kMaxEligibleContentTypes][kMaxContentTypeLen] = {};
    std::size_t eligible_content_type_count = 0;

    CompressionAlgorithm preferred_algorithms[kMaxCompressionAlgorithms] = {};
    std::size_t preferred_algorithm_count = 0;

    AlreadyEncodedBehavior already_encoded_behavior = AlreadyEncodedBehavior::Skip;
};

// Returns nullptr on success, or a static error string on invalid configuration.
const char* validate_compression_policy(const CompressionPolicy& policy);

} // namespace bytetaper::policy
