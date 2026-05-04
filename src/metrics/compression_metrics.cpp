// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/compression_metrics.h"

#include <cstdio>

namespace bytetaper::metrics {

void record_compression_event(CompressionMetrics* metrics, CompressionMetricEvent event) {
    if (metrics == nullptr)
        return;
    switch (event) {
    case CompressionMetricEvent::Candidate:
        metrics->candidate_total.fetch_add(1, std::memory_order_relaxed);
        break;
    case CompressionMetricEvent::SkipPolicyDisabled:
        metrics->skip_policy_disabled_total.fetch_add(1, std::memory_order_relaxed);
        break;
    case CompressionMetricEvent::SkipClientUnsupported:
        metrics->skip_client_unsupported_total.fetch_add(1, std::memory_order_relaxed);
        break;
    case CompressionMetricEvent::SkipAlreadyEncoded:
        metrics->skip_already_encoded_total.fetch_add(1, std::memory_order_relaxed);
        break;
    case CompressionMetricEvent::SkipContentType:
        metrics->skip_content_type_total.fetch_add(1, std::memory_order_relaxed);
        break;
    case CompressionMetricEvent::SkipResponseTooSmall:
        metrics->skip_response_too_small_total.fetch_add(1, std::memory_order_relaxed);
        break;
    case CompressionMetricEvent::SkipNon2xx:
        metrics->skip_non_2xx_total.fetch_add(1, std::memory_order_relaxed);
        break;
    }
}

std::size_t render_compression_metrics_prometheus(const CompressionMetrics& metrics, char* buf,
                                                  std::size_t buf_size) {
    if (buf == nullptr || buf_size == 0)
        return 0;

    const auto cand = metrics.candidate_total.load(std::memory_order_relaxed);
    const auto spd = metrics.skip_policy_disabled_total.load(std::memory_order_relaxed);
    const auto scu = metrics.skip_client_unsupported_total.load(std::memory_order_relaxed);
    const auto sae = metrics.skip_already_encoded_total.load(std::memory_order_relaxed);
    const auto sct = metrics.skip_content_type_total.load(std::memory_order_relaxed);
    const auto srs = metrics.skip_response_too_small_total.load(std::memory_order_relaxed);
    const auto sn2 = metrics.skip_non_2xx_total.load(std::memory_order_relaxed);

    const int written = std::snprintf(
        buf, buf_size,
        "# HELP bytetaper_compression_candidate_total Response eligibility evaluated as true\n"
        "# TYPE bytetaper_compression_candidate_total counter\n"
        "bytetaper_compression_candidate_total %llu\n"
        "# HELP bytetaper_compression_skip_policy_disabled_total Compression disabled by policy\n"
        "# TYPE bytetaper_compression_skip_policy_disabled_total counter\n"
        "bytetaper_compression_skip_policy_disabled_total %llu\n"
        "# HELP bytetaper_compression_skip_client_unsupported_total Client does not support "
        "compatible encoding\n"
        "# TYPE bytetaper_compression_skip_client_unsupported_total counter\n"
        "bytetaper_compression_skip_client_unsupported_total %llu\n"
        "# HELP bytetaper_compression_skip_already_encoded_total Response already has "
        "Content-Encoding\n"
        "# TYPE bytetaper_compression_skip_already_encoded_total counter\n"
        "bytetaper_compression_skip_already_encoded_total %llu\n"
        "# HELP bytetaper_compression_skip_content_type_total Content-Type not eligible for "
        "compression\n"
        "# TYPE bytetaper_compression_skip_content_type_total counter\n"
        "bytetaper_compression_skip_content_type_total %llu\n"
        "# HELP bytetaper_compression_skip_response_too_small_total Response body below "
        "min_size_bytes\n"
        "# TYPE bytetaper_compression_skip_response_too_small_total counter\n"
        "bytetaper_compression_skip_response_too_small_total %llu\n"
        "# HELP bytetaper_compression_skip_non_2xx_total Only 2xx responses are eligible\n"
        "# TYPE bytetaper_compression_skip_non_2xx_total counter\n"
        "bytetaper_compression_skip_non_2xx_total %llu\n",
        (unsigned long long) cand, (unsigned long long) spd, (unsigned long long) scu,
        (unsigned long long) sae, (unsigned long long) sct, (unsigned long long) srs,
        (unsigned long long) sn2);

    if (written < 0 || static_cast<std::size_t>(written) >= buf_size)
        return 0;
    return static_cast<std::size_t>(written);
}

} // namespace bytetaper::metrics
