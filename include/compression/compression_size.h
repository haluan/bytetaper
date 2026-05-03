// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstddef>
#include <cstdint>

namespace bytetaper::compression {

struct CompressionSizeResult {
    bool eligible = false;
    // Static reason string, or nullptr when eligible.
    // Possible values: "below_minimum", "size_unknown"
    const char* reason = nullptr;
};

// Checks whether body_len meets the min_size_bytes threshold.
// size_known must be false when body size is unavailable (chunked/streaming);
// returns eligible=false with reason="size_unknown" in that case.
// min_size_bytes=0 disables the size check — always eligible when size is known.
CompressionSizeResult check_compression_size_eligibility(std::uint32_t min_size_bytes,
                                                         std::size_t body_len, bool size_known);

} // namespace bytetaper::compression
