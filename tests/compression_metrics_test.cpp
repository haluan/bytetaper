// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/compression_metrics.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::metrics {

TEST(CompressionMetricsTest, Candidate_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::Candidate);
    EXPECT_EQ(m.candidate_total.load(), 1u);
}

TEST(CompressionMetricsTest, SkipPolicyDisabled_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::SkipPolicyDisabled);
    EXPECT_EQ(m.skip_policy_disabled_total.load(), 1u);
}

TEST(CompressionMetricsTest, SkipClientUnsupported_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::SkipClientUnsupported);
    EXPECT_EQ(m.skip_client_unsupported_total.load(), 1u);
}

TEST(CompressionMetricsTest, SkipAlreadyEncoded_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::SkipAlreadyEncoded);
    EXPECT_EQ(m.skip_already_encoded_total.load(), 1u);
}

TEST(CompressionMetricsTest, SkipContentType_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::SkipContentType);
    EXPECT_EQ(m.skip_content_type_total.load(), 1u);
}

TEST(CompressionMetricsTest, SkipResponseTooSmall_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::SkipResponseTooSmall);
    EXPECT_EQ(m.skip_response_too_small_total.load(), 1u);
}

TEST(CompressionMetricsTest, SkipNon2xx_Increments) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::SkipNon2xx);
    EXPECT_EQ(m.skip_non_2xx_total.load(), 1u);
}

TEST(CompressionMetricsTest, PrometheusRender_ContainsExpectedStrings) {
    CompressionMetrics m{};
    record_compression_event(&m, CompressionMetricEvent::Candidate);
    record_compression_event(&m, CompressionMetricEvent::SkipPolicyDisabled);

    char buf[4096];
    const std::size_t written = render_compression_metrics_prometheus(m, buf, sizeof(buf));
    ASSERT_GT(written, 0u);

    EXPECT_NE(std::strstr(buf, "bytetaper_compression_candidate_total 1"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_compression_skip_policy_disabled_total 1"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_compression_skip_client_unsupported_total 0"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_compression_skip_already_encoded_total 0"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_compression_skip_content_type_total 0"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_compression_skip_response_too_small_total 0"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_compression_skip_non_2xx_total 0"), nullptr);
}

TEST(CompressionMetricsTest, NullSafe) {
    record_compression_event(nullptr, CompressionMetricEvent::Candidate);
    // Should not crash
    EXPECT_EQ(render_compression_metrics_prometheus(CompressionMetrics{}, nullptr, 1024), 0u);
}

} // namespace bytetaper::metrics
