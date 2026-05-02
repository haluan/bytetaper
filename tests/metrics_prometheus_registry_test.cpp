// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/prometheus_registry.h"

#include <gtest/gtest.h>
#include <string>

namespace bytetaper::metrics {

TEST(PrometheusRegistryTest, RenderPayloadReductionRatio) {
    MetricsRegistry registry;

    // Test case 1: Standard reduction (20% savings)
    record_stream(&registry, { .original_bytes = 100, .optimized_bytes = 80 });
    std::string out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_payload_reduction_ratio 0.200000"), std::string::npos);

    // Test case 2: Accumulated reduction (1000 original, 600 optimized -> 40% savings)
    record_stream(&registry, { .original_bytes = 900, .optimized_bytes = 520 });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_payload_reduction_ratio 0.400000"), std::string::npos);
}

TEST(PrometheusRegistryTest, RenderPayloadReductionRatioZeroOriginal) {
    MetricsRegistry registry;

    // Initial state (zeros)
    std::string out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_payload_reduction_ratio 0.000000"), std::string::npos);

    // Record stream with zero bytes
    record_stream(&registry, { .original_bytes = 0, .optimized_bytes = 0 });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_payload_reduction_ratio 0.000000"), std::string::npos);
}

TEST(PrometheusRegistryTest, RenderPayloadReductionRatioNoSavings) {
    MetricsRegistry registry;

    // No savings (100 original, 100 optimized)
    record_stream(&registry, { .original_bytes = 100, .optimized_bytes = 100 });
    std::string out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_payload_reduction_ratio 0.000000"), std::string::npos);
}

TEST(PrometheusRegistryTest, RenderTransformErrors) {
    MetricsRegistry registry;

    // Initial state
    std::string out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_transform_errors_total 0"), std::string::npos);

    // Record stream with error
    record_stream(&registry, { .transform_error = true });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_transform_errors_total 1"), std::string::npos);

    // Record another error
    record_stream(&registry, { .transform_error = true });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_transform_errors_total 2"), std::string::npos);
}

TEST(PrometheusRegistryTest, RenderBytesSavedTotal) {
    MetricsRegistry registry;

    // Test case 1: Record 20 bytes savings
    record_stream(&registry, { .original_bytes = 100, .optimized_bytes = 80 });
    std::string out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_bytes_saved_total 20"), std::string::npos);

    // Test case 2: Record another 40 bytes savings (total 60)
    record_stream(&registry, { .original_bytes = 100, .optimized_bytes = 60 });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_bytes_saved_total 60"), std::string::npos);

    // Test case 3: No savings (should not increment)
    record_stream(&registry, { .original_bytes = 100, .optimized_bytes = 100 });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_bytes_saved_total 60"), std::string::npos);

    // Test case 4: Negative savings / expansion (clamped at 0 per request)
    record_stream(&registry, { .original_bytes = 100, .optimized_bytes = 120 });
    out = render_prometheus_text(registry);
    EXPECT_NE(out.find("bytetaper_bytes_saved_total 60"), std::string::npos);
}

TEST(PrometheusRegistryTest, RenderRouteWasteMetric) {
    MetricsRegistry registry;

    // Route 1: 20 bytes saved
    record_stream(&registry,
                  { .original_bytes = 100, .optimized_bytes = 80, .route_id = "route-a" });
    // Route 2: 50 bytes saved
    record_stream(&registry,
                  { .original_bytes = 100, .optimized_bytes = 50, .route_id = "route-b" });
    // Route 1: another 10 bytes saved
    record_stream(&registry,
                  { .original_bytes = 50, .optimized_bytes = 40, .route_id = "route-a" });

    std::string out = render_prometheus_text(registry);

    EXPECT_NE(out.find("bytetaper_route_waste_saved_bytes_total{route=\"route-a\"} 30"),
              std::string::npos);
    EXPECT_NE(out.find("bytetaper_route_waste_saved_bytes_total{route=\"route-b\"} 50"),
              std::string::npos);
}

} // namespace bytetaper::metrics
