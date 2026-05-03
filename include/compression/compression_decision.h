// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include "compression/accept_encoding.h"
#include "compression/content_encoding.h"
#include "policy/compression_policy.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::compression {

struct CompressionDecisionInput {
    const policy::CompressionPolicy* compression_policy = nullptr; // required
    AcceptEncoding client_encoding{};          // parsed from request Accept-Encoding
    ContentEncodingResult response_encoding{}; // parsed from response Content-Encoding
    std::uint16_t status_code = 0;
    const char* content_type = nullptr;
    std::size_t content_type_len = 0;
    std::size_t body_len = 0;
    bool body_size_known = false;
};

struct CompressionDecision {
    bool candidate = false;
    // Static reason string, or nullptr when candidate=true.
    // Values: "policy_disabled", "no_client_support", "non_2xx_status",
    //         "already_encoded", "content_type_not_eligible",
    //         "below_minimum", "size_unknown"
    const char* reason = nullptr;
    // Suggested algorithm for Envoy; None when candidate=false or no match found.
    policy::CompressionAlgorithm selected_algorithm_hint = policy::CompressionAlgorithm::None;
};

CompressionDecision make_compression_decision(const CompressionDecisionInput& input);

} // namespace bytetaper::compression
