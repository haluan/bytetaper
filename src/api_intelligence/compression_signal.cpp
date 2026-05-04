// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "api_intelligence/compression_signal.h"

namespace bytetaper::api_intelligence {

CompressionOpportunity assess_compression_opportunity(const CompressionSignals& signals) {
    if (signals.already_encoded_seen) {
        return { false, nullptr };
    }
    if (signals.compression_skipped_too_small_seen) {
        return { false, "response_too_small" };
    }
    if (signals.large_uncompressed_response_seen) {
        return { true, "large_uncompressed_json_response_seen" };
    }
    return { false, nullptr };
}

} // namespace bytetaper::api_intelligence
