// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/grpc_server.h"

#include <chrono>
#include <cstdint>
#include <string>

#include <grpcpp/grpcpp.h>

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"

int main() {
    bytetaper::extproc::GrpcServerConfig config{};
    bytetaper::extproc::GrpcServerHandle handle{};

    if (!bytetaper::extproc::start_grpc_server(config, &handle)) {
        return 1;
    }
    if (handle.bound_port == 0) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 2;
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(static_cast<std::uint32_t>(handle.bound_port));
    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    if (!channel) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 3;
    }

    const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    if (!channel->WaitForConnected(deadline)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 4;
    }

    auto stub = envoy::service::ext_proc::v3::ExternalProcessor::NewStub(channel);
    if (!stub) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 5;
    }

    grpc::ClientContext client_context{};
    auto stream = stub->Process(&client_context);
    if (!stream) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 6;
    }

    envoy::service::ext_proc::v3::ProcessingRequest request_headers{};
    request_headers.mutable_request_headers();
    if (!stream->Write(request_headers)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 7;
    }

    envoy::service::ext_proc::v3::ProcessingRequest response_headers{};
    response_headers.mutable_response_headers();
    if (!stream->Write(response_headers)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 8;
    }

    envoy::service::ext_proc::v3::ProcessingRequest unsupported{};
    unsupported.mutable_response_body();
    if (!stream->Write(unsupported)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 9;
    }

    if (!stream->WritesDone()) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 10;
    }

    envoy::service::ext_proc::v3::ProcessingResponse response{};
    if (stream->Read(&response)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 11;
    }

    const grpc::Status status = stream->Finish();

    bytetaper::extproc::stop_grpc_server(&handle);

    if (!status.ok()) {
        return 12;
    }

    return 0;
}
