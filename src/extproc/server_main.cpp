// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/grpc_server.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <thread>

namespace {

std::atomic<bool> g_should_stop{ false };

void on_signal(int) {
    g_should_stop.store(true);
}

const char* parse_listen_address(int argc, char** argv) {
    const char* listen_address = "0.0.0.0:18080";

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }

        if (std::strcmp(arg, "--listen-address") == 0) {
            if (i + 1 >= argc || argv[i + 1] == nullptr || argv[i + 1][0] == '\0') {
                std::fprintf(stderr, "missing value for --listen-address\n");
                return nullptr;
            }
            listen_address = argv[i + 1];
            i += 1;
            continue;
        }

        if (std::strcmp(arg, "--help") == 0) {
            std::puts("usage: bytetaper-extproc-server [--listen-address HOST:PORT]");
            return "";
        }

        std::fprintf(stderr, "unknown argument: %s\n", arg);
        return nullptr;
    }

    return listen_address;
}

} // namespace

int main(int argc, char** argv) {
    const char* listen_address = parse_listen_address(argc, argv);
    if (listen_address == nullptr) {
        return 2;
    }
    if (listen_address[0] == '\0') {
        return 0;
    }

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    bytetaper::extproc::GrpcServerConfig config{};
    config.listen_address = listen_address;

    bytetaper::extproc::GrpcServerHandle handle{};
    if (!bytetaper::extproc::start_grpc_server(config, &handle)) {
        std::fprintf(stderr, "failed to start extproc server\n");
        return 1;
    }

    while (!g_should_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
