// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_METRICS_HTTP_SERVER_H
#define BYTETAPER_METRICS_HTTP_SERVER_H

#include "metrics/prometheus_registry.h"

#include <cstdint>

namespace bytetaper::metrics {

struct MetricsHttpServerConfig {
    const char* listen_address = "127.0.0.1";
    std::uint16_t port = 0; // 0 = OS auto-assign
    MetricsRegistry* registry = nullptr;
};

struct MetricsHttpServerHandle {
    void* impl = nullptr;
    std::uint16_t bound_port = 0;
};

bool start_metrics_http_server(const MetricsHttpServerConfig& config,
                               MetricsHttpServerHandle* handle);
void stop_metrics_http_server(MetricsHttpServerHandle* handle);

} // namespace bytetaper::metrics

#endif // BYTETAPER_METRICS_HTTP_SERVER_H
