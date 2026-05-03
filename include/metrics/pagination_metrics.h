// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace bytetaper::metrics {

enum class PaginationMetricEvent : std::uint8_t {
    DefaultLimitApplied,     // missing limit → default injected
    MaxLimitEnforced,        // client limit capped to max_limit
    ValidLimit,              // client limit passed validation unchanged
    UpstreamUnsupported,     // pagination skipped: upstream_supports_pagination=false
    OversizedResponseWarning // response body exceeded max_response_bytes_warning
};

struct PaginationMetrics {
    std::atomic<std::uint64_t> default_limit_applied{ 0 };
    std::atomic<std::uint64_t> max_limit_enforced{ 0 };
    std::atomic<std::uint64_t> valid_limit{ 0 };
    std::atomic<std::uint64_t> upstream_unsupported{ 0 };
    std::atomic<std::uint64_t> oversized_response_warning{ 0 };
};

void record_pagination_event(PaginationMetrics* metrics, PaginationMetricEvent event);

// Renders all five counters as Prometheus text into buf.
// Returns bytes written (excluding null terminator), or 0 on overflow / null buf.
std::size_t render_pagination_metrics_prometheus(const PaginationMetrics& metrics, char* buf,
                                                 std::size_t buf_size);

} // namespace bytetaper::metrics
