// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/grpc_server.h"
#include "extproc/request_runtime.h"

#include <memory>

#include <grpcpp/grpcpp.h>

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"

namespace bytetaper::extproc {

namespace {

class ExternalProcessorSkeletonService final
    : public envoy::service::ext_proc::v3::ExternalProcessor::Service {
public:
    grpc::Status Process(grpc::ServerContext*,
                         grpc::ServerReaderWriter<envoy::service::ext_proc::v3::ProcessingResponse,
                                                  envoy::service::ext_proc::v3::ProcessingRequest>*
                             stream) override {
        if (stream == nullptr) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "missing stream");
        }

        ProcessingStreamStats stream_stats{};
        envoy::service::ext_proc::v3::ProcessingRequest request{};
        while (stream->Read(&request)) {
            const ProcessingRequestKind kind =
                classify_request_kind(request.has_request_headers(), request.has_response_headers(),
                                      request.has_response_body());
            record_request_kind(kind, &stream_stats);

            if (kind == ProcessingRequestKind::RequestHeaders) {
                continue;
            }
            if (kind == ProcessingRequestKind::ResponseHeaders) {
                continue;
            }
            // Non-supported variants are safe no-op; response_body support is deferred.
        }

        (void) stream_stats;
        return grpc::Status::OK;
    }
};

struct GrpcServerImpl {
    ExternalProcessorSkeletonService service{};
    std::unique_ptr<grpc::Server> server{};
};

} // namespace

bool start_grpc_server(const GrpcServerConfig& config, GrpcServerHandle* handle) {
    if (handle == nullptr) {
        return false;
    }
    if (handle->impl != nullptr) {
        return false;
    }
    if (config.listen_address == nullptr) {
        return false;
    }

    auto impl = std::make_unique<GrpcServerImpl>();

    grpc::ServerBuilder builder{};
    int selected_port = 0;
    builder.AddListeningPort(config.listen_address, grpc::InsecureServerCredentials(),
                             &selected_port);
    builder.RegisterService(&impl->service);

    impl->server = builder.BuildAndStart();
    if (!impl->server) {
        return false;
    }
    if (selected_port <= 0 || selected_port > 65535) {
        impl->server->Shutdown();
        impl->server->Wait();
        return false;
    }

    handle->bound_port = static_cast<std::uint16_t>(selected_port);
    handle->impl = impl.release();
    return true;
}

void stop_grpc_server(GrpcServerHandle* handle) {
    if (handle == nullptr) {
        return;
    }
    if (handle->impl == nullptr) {
        handle->bound_port = 0;
        return;
    }

    auto* impl = static_cast<GrpcServerImpl*>(handle->impl);
    if (impl->server) {
        impl->server->Shutdown();
        impl->server->Wait();
    }

    delete impl;
    handle->impl = nullptr;
    handle->bound_port = 0;
}

} // namespace bytetaper::extproc
