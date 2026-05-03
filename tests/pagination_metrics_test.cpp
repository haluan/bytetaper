// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/pagination_metrics.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::metrics {

TEST(PaginationMetricsTest, DefaultLimitApplied_Increments) {
    PaginationMetrics m{};
    record_pagination_event(&m, PaginationMetricEvent::DefaultLimitApplied);
    EXPECT_EQ(m.default_limit_applied.load(), 1u);
}

TEST(PaginationMetricsTest, MaxLimitEnforced_Increments) {
    PaginationMetrics m{};
    record_pagination_event(&m, PaginationMetricEvent::MaxLimitEnforced);
    EXPECT_EQ(m.max_limit_enforced.load(), 1u);
}

TEST(PaginationMetricsTest, ValidLimit_Increments) {
    PaginationMetrics m{};
    record_pagination_event(&m, PaginationMetricEvent::ValidLimit);
    EXPECT_EQ(m.valid_limit.load(), 1u);
}

TEST(PaginationMetricsTest, UpstreamUnsupported_Increments) {
    PaginationMetrics m{};
    record_pagination_event(&m, PaginationMetricEvent::UpstreamUnsupported);
    EXPECT_EQ(m.upstream_unsupported.load(), 1u);
}

TEST(PaginationMetricsTest, OversizedResponseWarning_Increments) {
    PaginationMetrics m{};
    record_pagination_event(&m, PaginationMetricEvent::OversizedResponseWarning);
    EXPECT_EQ(m.oversized_response_warning.load(), 1u);
}

TEST(PaginationMetricsTest, PrometheusRender_ContainsAllNames) {
    PaginationMetrics m{};
    record_pagination_event(&m, PaginationMetricEvent::DefaultLimitApplied);
    record_pagination_event(&m, PaginationMetricEvent::MaxLimitEnforced);
    char buf[4096];
    const std::size_t written = render_pagination_metrics_prometheus(m, buf, sizeof(buf));
    ASSERT_GT(written, 0u);
    EXPECT_NE(std::strstr(buf, "bytetaper_pagination_default_limit_applied_total 1"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_pagination_max_limit_enforced_total 1"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_pagination_valid_limit_total 0"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_pagination_upstream_unsupported_total 0"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_pagination_oversized_response_warning_total 0"), nullptr);
}

TEST(PaginationMetricsTest, NullSafe) {
    record_pagination_event(nullptr, PaginationMetricEvent::DefaultLimitApplied);
    char buf[4096];
    // render with null buf returns 0 gracefully
    EXPECT_EQ(render_pagination_metrics_prometheus(PaginationMetrics{}, nullptr, 4096), 0u);
}

} // namespace bytetaper::metrics
