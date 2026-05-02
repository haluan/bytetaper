// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "metrics/metrics_http_server.h"
#include "metrics/prometheus_registry.h"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    using namespace bytetaper::metrics;

    // 1. Create registry and record data
    MetricsRegistry registry;
    record_stream(&registry, { .original_bytes = 100,
                               .optimized_bytes = 80,
                               .removed_fields = 2,
                               .transform_applied = true });

    // 2. Start server
    MetricsHttpServerConfig config;
    config.listen_address = "127.0.0.1";
    config.port = 0;
    config.registry = &registry;

    MetricsHttpServerHandle handle;
    if (!start_metrics_http_server(config, &handle)) {
        std::cerr << "Failed to start metrics server" << std::endl;
        return 1;
    }

    if (handle.bound_port == 0) {
        std::cerr << "Bound port is 0" << std::endl;
        stop_metrics_http_server(&handle);
        return 2;
    }

    // 3. Connect via raw POSIX socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        stop_metrics_http_server(&handle);
        return 3;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(handle.bound_port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(sock);
        stop_metrics_http_server(&handle);
        return 4;
    }

    // 4. Send GET /metrics
    std::string request = "GET /metrics HTTP/1.0\r\nHost: localhost\r\n\r\n";
    send(sock, request.data(), request.size(), 0);

    // 5. Read response
    std::string response;
    char buffer[1024];
    while (true) {
        ssize_t n = read(sock, buffer, sizeof(buffer));
        if (n <= 0)
            break;
        response.append(buffer, n);
    }
    close(sock);

    // 6. Assertions
    if (response.find("bytetaper_streams_total 1") == std::string::npos) {
        std::cerr << "Missing bytetaper_streams_total 1" << std::endl;
        std::cerr << "Response was: " << response << std::endl;
        stop_metrics_http_server(&handle);
        return 100;
    }

    if (response.find("bytetaper_bytes_original_total 100") == std::string::npos) {
        std::cerr << "Missing bytetaper_bytes_original_total 100" << std::endl;
        stop_metrics_http_server(&handle);
        return 101;
    }

    if (response.find("bytetaper_transform_applied_total 1") == std::string::npos) {
        std::cerr << "Missing bytetaper_transform_applied_total 1" << std::endl;
        stop_metrics_http_server(&handle);
        return 102;
    }

    // 7. Cleanup
    stop_metrics_http_server(&handle);
    std::cout << "Test passed!" << std::endl;
    return 0;
}
