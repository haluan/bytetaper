// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <cstdint>

namespace bytetaper::api_intelligence {

struct CompressionSignals {
    bool large_uncompressed_response_seen = false;
    bool client_supports_compression_seen = false;
    bool already_encoded_seen = false;
    bool compression_candidate_seen = false;
    bool compression_skipped_too_small_seen = false;
};

struct CompressionOpportunity {
    bool has_opportunity = false;
    const char* recommendation_reason = nullptr;
};

// Pure function: maps observed compression signals to an opportunity for API intelligence
// reporting.
CompressionOpportunity assess_compression_opportunity(const CompressionSignals& signals);

} // namespace bytetaper::api_intelligence
