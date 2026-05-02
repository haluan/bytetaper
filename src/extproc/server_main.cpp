// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/grpc_server.h"
#include "policy/yaml_loader.h"

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

struct ServerArgs {
    const char* listen_address = "0.0.0.0:18080";
    const char* policy_file = nullptr;
    bool help = false;
    bool error = false;
};

ServerArgs parse_args(int argc, char** argv) {
    ServerArgs args{};

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }

        if (std::strcmp(arg, "--listen-address") == 0) {
            if (i + 1 >= argc || argv[i + 1] == nullptr || argv[i + 1][0] == '\0') {
                std::fprintf(stderr, "missing value for --listen-address\n");
                args.error = true;
                return args;
            }
            args.listen_address = argv[i + 1];
            i += 1;
            continue;
        }

        if (std::strcmp(arg, "--policy-file") == 0) {
            if (i + 1 >= argc || argv[i + 1] == nullptr || argv[i + 1][0] == '\0') {
                std::fprintf(stderr, "missing value for --policy-file\n");
                args.error = true;
                return args;
            }
            args.policy_file = argv[i + 1];
            i += 1;
            continue;
        }

        if (std::strcmp(arg, "--help") == 0) {
            args.help = true;
            return args;
        }

        std::fprintf(stderr, "unknown argument: %s\n", arg);
        args.error = true;
        return args;
    }

    return args;
}

} // namespace

int main(int argc, char** argv) {
    ServerArgs args = parse_args(argc, argv);
    if (args.error) {
        return 2;
    }
    if (args.help) {
        std::puts(
            "usage: bytetaper-extproc-server [--listen-address HOST:PORT] [--policy-file PATH]");
        return 0;
    }

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    bytetaper::policy::PolicyFileResult policy_result{};
    if (args.policy_file != nullptr) {
        if (!bytetaper::policy::load_policy_from_file(args.policy_file, &policy_result)) {
            std::fprintf(stderr, "failed to load policy file %s: %s\n", args.policy_file,
                         policy_result.error ? policy_result.error : "unknown error");
            return 3;
        }
        std::printf("loaded %zu routes from %s\n", policy_result.count, args.policy_file);
    }

    bytetaper::extproc::GrpcServerConfig config{};
    config.listen_address = args.listen_address;
    if (policy_result.ok) {
        config.policies = policy_result.policies;
        config.policy_count = policy_result.count;
    }

    bytetaper::extproc::GrpcServerHandle handle{};
    if (!bytetaper::extproc::start_grpc_server(config, &handle)) {
        std::fprintf(stderr, "failed to start extproc server on %s\n", args.listen_address);
        return 1;
    }

    std::printf("bytetaper-extproc-server listening on %s\n", args.listen_address);

    while (!g_should_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
