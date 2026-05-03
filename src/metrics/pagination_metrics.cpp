// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/pagination_metrics.h"

#include <cstdio>

namespace bytetaper::metrics {

void record_pagination_event(PaginationMetrics* metrics, PaginationMetricEvent event) {
    if (metrics == nullptr)
        return;
    switch (event) {
    case PaginationMetricEvent::DefaultLimitApplied:
        metrics->default_limit_applied.fetch_add(1, std::memory_order_relaxed);
        break;
    case PaginationMetricEvent::MaxLimitEnforced:
        metrics->max_limit_enforced.fetch_add(1, std::memory_order_relaxed);
        break;
    case PaginationMetricEvent::ValidLimit:
        metrics->valid_limit.fetch_add(1, std::memory_order_relaxed);
        break;
    case PaginationMetricEvent::UpstreamUnsupported:
        metrics->upstream_unsupported.fetch_add(1, std::memory_order_relaxed);
        break;
    case PaginationMetricEvent::OversizedResponseWarning:
        metrics->oversized_response_warning.fetch_add(1, std::memory_order_relaxed);
        break;
    }
}

std::size_t render_pagination_metrics_prometheus(const PaginationMetrics& metrics, char* buf,
                                                 std::size_t buf_size) {
    if (buf == nullptr || buf_size == 0)
        return 0;

    const auto dla = metrics.default_limit_applied.load(std::memory_order_relaxed);
    const auto mle = metrics.max_limit_enforced.load(std::memory_order_relaxed);
    const auto vl = metrics.valid_limit.load(std::memory_order_relaxed);
    const auto uu = metrics.upstream_unsupported.load(std::memory_order_relaxed);
    const auto orw = metrics.oversized_response_warning.load(std::memory_order_relaxed);

    const int written = std::snprintf(
        buf, buf_size,
        "# HELP bytetaper_pagination_default_limit_applied_total Default limit injected for "
        "missing client limit\n"
        "# TYPE bytetaper_pagination_default_limit_applied_total counter\n"
        "bytetaper_pagination_default_limit_applied_total %llu\n"
        "# HELP bytetaper_pagination_max_limit_enforced_total Client limit capped to policy max\n"
        "# TYPE bytetaper_pagination_max_limit_enforced_total counter\n"
        "bytetaper_pagination_max_limit_enforced_total %llu\n"
        "# HELP bytetaper_pagination_valid_limit_total Client limit passed validation unchanged\n"
        "# TYPE bytetaper_pagination_valid_limit_total counter\n"
        "bytetaper_pagination_valid_limit_total %llu\n"
        "# HELP bytetaper_pagination_upstream_unsupported_total Pagination skipped: upstream does "
        "not support pagination\n"
        "# TYPE bytetaper_pagination_upstream_unsupported_total counter\n"
        "bytetaper_pagination_upstream_unsupported_total %llu\n"
        "# HELP bytetaper_pagination_oversized_response_warning_total Response body exceeded "
        "max_response_bytes_warning\n"
        "# TYPE bytetaper_pagination_oversized_response_warning_total counter\n"
        "bytetaper_pagination_oversized_response_warning_total %llu\n",
        (unsigned long long) dla, (unsigned long long) mle, (unsigned long long) vl,
        (unsigned long long) uu, (unsigned long long) orw);

    if (written < 0 || static_cast<std::size_t>(written) >= buf_size)
        return 0;
    return static_cast<std::size_t>(written);
}

} // namespace bytetaper::metrics
