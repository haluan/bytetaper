// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"
#include "extproc/grpc_server.h"

#include <chrono>
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <string>

// Input: {"id":1,"name":"Alice"} = 23 bytes (two fields, flat JSON)
// Filter: ?fields=id removes "name", output: {"id":1} = 8 bytes
static constexpr const char* kInputBody = "{\"id\":1,\"name\":\"Alice\"}";
static constexpr std::size_t kExpectedOptimizedBytes = 8;

int main() {
    bytetaper::policy::RoutePolicy p1{};
    p1.route_id = "test-policy";
    p1.match_kind = bytetaper::policy::RouteMatchKind::Prefix;
    p1.match_prefix = "/api/";
    p1.mutation = bytetaper::policy::MutationMode::Full;

    bytetaper::extproc::GrpcServerConfig config{};
    config.policies = &p1;
    config.policy_count = 1;
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

    // request_headers: path with ?fields=id → has_query_selection=true → filtered path
    envoy::service::ext_proc::v3::ProcessingRequest request_headers{};
    auto* req_hdrs = request_headers.mutable_request_headers()->mutable_headers();
    auto* path_hdr = req_hdrs->add_headers();
    path_hdr->set_key(":path");
    path_hdr->set_raw_value("/api/items?fields=id");
    if (!stream->Write(request_headers)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 7;
    }

    // response_headers: application/json → EligibleJson
    envoy::service::ext_proc::v3::ProcessingRequest response_headers{};
    auto* resp_hdrs = response_headers.mutable_response_headers()->mutable_headers();
    auto* ct_hdr = resp_hdrs->add_headers();
    ct_hdr->set_key("content-type");
    ct_hdr->set_raw_value("application/json");
    if (!stream->Write(response_headers)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 8;
    }

    // response_body: 23-byte two-field JSON, end_of_stream=true → triggers filtering
    envoy::service::ext_proc::v3::ProcessingRequest response_body{};
    response_body.mutable_response_body()->set_body(kInputBody);
    response_body.mutable_response_body()->set_end_of_stream(true);
    if (!stream->Write(response_body)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 9;
    }

    if (!stream->WritesDone()) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 10;
    }

    // Response 1: request_headers continue
    envoy::service::ext_proc::v3::ProcessingResponse r1{};
    if (!stream->Read(&r1)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 11;
    }
    if (!r1.has_request_headers()) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 12;
    }

    // Response 2: response_headers continue
    envoy::service::ext_proc::v3::ProcessingResponse r2{};
    if (!stream->Read(&r2)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 13;
    }
    if (!r2.has_response_headers()) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 14;
    }

    // Response 3: response_body — must contain x-bytetaper-optimized-response-bytes == "8"
    envoy::service::ext_proc::v3::ProcessingResponse r3{};
    if (!stream->Read(&r3)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 15;
    }
    if (!r3.has_response_body()) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 16;
    }
    {
        const std::string expected_value = std::to_string(kExpectedOptimizedBytes);
        bool found = false;
        for (const auto& mutation : r3.response_body().response().header_mutation().set_headers()) {
            if (mutation.header().key() == "x-bytetaper-optimized-response-bytes" &&
                mutation.header().raw_value() == expected_value) {
                found = true;
                break;
            }
        }
        if (!found) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 100;
        }
    }

    const grpc::Status status = stream->Finish();

    bytetaper::extproc::stop_grpc_server(&handle);

    if (!status.ok()) {
        return 17;
    }

    return 0;
}
