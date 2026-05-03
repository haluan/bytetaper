// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "compression/compression_decision.h"

#include "compression/compression_eligibility.h"
#include "compression/compression_size.h"

namespace bytetaper::compression {

namespace {

// Returns the best algorithm supported by the client given policy preferences.
// Prefers policy preferred_algorithms in order; falls back to br > zstd > gzip.
static policy::CompressionAlgorithm pick_algorithm(const policy::CompressionPolicy& policy,
                                                   const AcceptEncoding& client) {

    // Try policy-preferred order first.
    for (std::size_t i = 0; i < policy.preferred_algorithm_count; ++i) {
        const auto alg = policy.preferred_algorithms[i];
        if (alg == policy::CompressionAlgorithm::Brotli && client.supports_br)
            return alg;
        if (alg == policy::CompressionAlgorithm::Zstd && client.supports_zstd)
            return alg;
        if (alg == policy::CompressionAlgorithm::Gzip && client.supports_gzip)
            return alg;
    }
    // Fallback priority: br > zstd > gzip.
    if (client.supports_br)
        return policy::CompressionAlgorithm::Brotli;
    if (client.supports_zstd)
        return policy::CompressionAlgorithm::Zstd;
    if (client.supports_gzip)
        return policy::CompressionAlgorithm::Gzip;
    return policy::CompressionAlgorithm::None;
}

} // namespace

CompressionDecision make_compression_decision(const CompressionDecisionInput& input) {
    if (input.compression_policy == nullptr || !input.compression_policy->enabled) {
        return { false, "policy_disabled" };
    }

    const auto& policy = *input.compression_policy;
    const auto& client = input.client_encoding;

    if (!client.supports_gzip && !client.supports_br && !client.supports_zstd) {
        return { false, "no_client_support" };
    }

    if (input.status_code < 200 || input.status_code >= 300) {
        return { false, "non_2xx_status" };
    }

    if (input.response_encoding.already_encoded) {
        return { false, "already_encoded" };
    }

    if (!is_content_type_eligible(policy, input.content_type, input.content_type_len)) {
        return { false, "content_type_not_eligible" };
    }

    const auto size_result = check_compression_size_eligibility(
        policy.min_size_bytes, input.body_len, input.body_size_known);
    if (!size_result.eligible) {
        return { false, size_result.reason };
    }

    const auto alg = pick_algorithm(policy, client);
    return { true, nullptr, alg };
}

} // namespace bytetaper::compression
