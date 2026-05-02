// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_METRICS_PROMETHEUS_REGISTRY_H
#define BYTETAPER_METRICS_PROMETHEUS_REGISTRY_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace bytetaper::metrics {

struct RouteWasteMetric {
    std::string route_id;
    std::atomic<std::uint64_t> waste_saved_bytes{ 0 };

    explicit RouteWasteMetric(std::string_view id) : route_id(id) {}
    RouteWasteMetric(const RouteWasteMetric&) = delete;
    RouteWasteMetric& operator=(const RouteWasteMetric&) = delete;
};

struct MetricsRegistry {
    std::atomic<std::uint64_t> streams_total{ 0 };
    std::atomic<std::uint64_t> bytes_original_total{ 0 };
    std::atomic<std::uint64_t> bytes_optimized_total{ 0 };
    std::atomic<std::uint64_t> fields_removed_total{ 0 };
    std::atomic<std::uint64_t> transform_applied_total{ 0 };
    std::atomic<std::uint64_t> transform_errors_total{ 0 };
    std::atomic<std::uint64_t> bytes_saved_total{ 0 };

    mutable std::mutex routes_mutex;
    std::vector<std::unique_ptr<RouteWasteMetric>> route_metrics;
};

struct StreamRecord {
    std::uint64_t original_bytes = 0;
    std::uint64_t optimized_bytes = 0;
    std::uint64_t removed_fields = 0;
    bool transform_applied = false;
    bool transform_error = false;
    std::string_view route_id = "";
};

void record_stream(MetricsRegistry* registry, const StreamRecord& record);
std::string render_prometheus_text(const MetricsRegistry& registry);

} // namespace bytetaper::metrics

#endif // BYTETAPER_METRICS_PROMETHEUS_REGISTRY_H
