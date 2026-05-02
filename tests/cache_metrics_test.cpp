// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/cache_metrics.h"

#include <cstring>
#include <gtest/gtest.h>

namespace bytetaper::metrics {

TEST(CacheMetricsTest, L1HitIncrementsCounter) {
    CacheMetrics m{};
    record_cache_event(&m, CacheMetricEvent::L1Hit);
    record_cache_event(&m, CacheMetricEvent::L1Hit);
    EXPECT_EQ(m.l1_hit.load(), 2u);
    EXPECT_EQ(m.l1_miss.load(), 0u);
}

TEST(CacheMetricsTest, L2HitIncrementsCounter) {
    CacheMetrics m{};
    record_cache_event(&m, CacheMetricEvent::L2Hit);
    EXPECT_EQ(m.l2_hit.load(), 1u);
    EXPECT_EQ(m.l2_miss.load(), 0u);
}

TEST(CacheMetricsTest, ExpiredIncrementsCounter) {
    CacheMetrics m{};
    record_cache_event(&m, CacheMetricEvent::Expired);
    EXPECT_EQ(m.expired.load(), 1u);
}

TEST(CacheMetricsTest, BypassIncrementsCounter) {
    CacheMetrics m{};
    record_cache_event(&m, CacheMetricEvent::Bypass);
    EXPECT_EQ(m.bypass.load(), 1u);
}

TEST(CacheMetricsTest, PrometheusRenderContainsAllCounters) {
    CacheMetrics m{};
    record_cache_event(&m, CacheMetricEvent::L1Hit);
    record_cache_event(&m, CacheMetricEvent::L2Miss);
    record_cache_event(&m, CacheMetricEvent::Bypass);
    char buf[4096]{};
    std::size_t n = render_cache_metrics_prometheus(m, buf, sizeof(buf));
    ASSERT_GT(n, 0u);
    EXPECT_NE(std::strstr(buf, "bytetaper_cache_l1_hit_total 1"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_cache_l2_miss_total 1"), nullptr);
    EXPECT_NE(std::strstr(buf, "bytetaper_cache_bypass_total 1"), nullptr);
}

} // namespace bytetaper::metrics
