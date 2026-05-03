// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/compression_size.h"

namespace bytetaper::compression {

CompressionSizeResult check_compression_size_eligibility(std::uint32_t min_size_bytes,
                                                         std::size_t body_len, bool size_known) {
    if (!size_known) {
        return { false, "size_unknown" };
    }
    if (min_size_bytes > 0 && body_len < static_cast<std::size_t>(min_size_bytes)) {
        return { false, "below_minimum" };
    }
    return { true, nullptr };
}

} // namespace bytetaper::compression
