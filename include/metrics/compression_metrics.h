// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace bytetaper::metrics {

enum class CompressionMetricEvent : std::uint8_t {
    Candidate,             // Compression eligibility evaluated as true
    SkipPolicyDisabled,    // Compression disabled by policy
    SkipClientUnsupported, // Client does not support compatible encoding
    SkipAlreadyEncoded,    // Response already has Content-Encoding
    SkipContentType,       // Content-Type not eligible for compression
    SkipResponseTooSmall,  // Response body below min_size_bytes
    SkipNon2xx             // Only 2xx responses are eligible
};

struct CompressionMetrics {
    std::atomic<std::uint64_t> candidate_total{ 0 };
    std::atomic<std::uint64_t> skip_policy_disabled_total{ 0 };
    std::atomic<std::uint64_t> skip_client_unsupported_total{ 0 };
    std::atomic<std::uint64_t> skip_already_encoded_total{ 0 };
    std::atomic<std::uint64_t> skip_content_type_total{ 0 };
    std::atomic<std::uint64_t> skip_response_too_small_total{ 0 };
    std::atomic<std::uint64_t> skip_non_2xx_total{ 0 };
};

void record_compression_event(CompressionMetrics* metrics, CompressionMetricEvent event);

// Renders all counters as Prometheus text into buf.
// Returns bytes written (excluding null terminator), or 0 on overflow / null buf.
std::size_t render_compression_metrics_prometheus(const CompressionMetrics& metrics, char* buf,
                                                  std::size_t buf_size);

} // namespace bytetaper::metrics
