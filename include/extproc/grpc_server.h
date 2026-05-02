// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_EXTPROC_GRPC_SERVER_H
#define BYTETAPER_EXTPROC_GRPC_SERVER_H

#include <cstddef>
#include <cstdint>

#include "policy/route_policy.h"

namespace bytetaper::extproc {

struct GrpcServerConfig {
    const char* listen_address = "127.0.0.1:0";
    const policy::RoutePolicy* policies = nullptr;
    std::size_t policy_count = 0;
};

struct GrpcServerHandle {
    void* impl = nullptr;
    std::uint16_t bound_port = 0;
};

bool start_grpc_server(const GrpcServerConfig& config, GrpcServerHandle* handle);
void stop_grpc_server(GrpcServerHandle* handle);

} // namespace bytetaper::extproc

#endif // BYTETAPER_EXTPROC_GRPC_SERVER_H
