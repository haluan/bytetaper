// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"
#include "extproc/grpc_server.h"
#include "policy/route_policy.h"

#include <chrono>
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

// Stream 1: /api/items matches prefix /api/ → x-bytetaper-route-policy must be "api-v1"
// Stream 2: /other/path does not match     → x-bytetaper-route-policy must be "none"
static constexpr const char* kExpectedMatch = "api-v1";
static constexpr const char* kExpectedNone = "none";

// Returns true if x-bytetaper-route-policy in r2 equals expected_policy.
static bool run_one_stream(const std::shared_ptr<grpc::Channel>& channel, const char* path,
                           const char* expected_policy) {
    auto stub = envoy::service::ext_proc::v3::ExternalProcessor::NewStub(channel);
    if (!stub) {
        return false;
    }

    grpc::ClientContext client_context{};
    auto stream = stub->Process(&client_context);
    if (!stream) {
        return false;
    }

    // request_headers
    envoy::service::ext_proc::v3::ProcessingRequest req_hdrs{};
    auto* hdr_map = req_hdrs.mutable_request_headers()->mutable_headers();
    auto* path_hdr = hdr_map->add_headers();
    path_hdr->set_key(":path");
    path_hdr->set_raw_value(path);
    if (!stream->Write(req_hdrs)) {
        return false;
    }

    // response_headers (triggers ResponseHeaders phase where the header is emitted)
    envoy::service::ext_proc::v3::ProcessingRequest resp_hdrs{};
    auto* resp_hdr_map = resp_hdrs.mutable_response_headers()->mutable_headers();
    auto* ct_hdr = resp_hdr_map->add_headers();
    ct_hdr->set_key("content-type");
    ct_hdr->set_raw_value("application/json");
    if (!stream->Write(resp_hdrs)) {
        return false;
    }

    if (!stream->WritesDone()) {
        return false;
    }

    // r1: request_headers
    envoy::service::ext_proc::v3::ProcessingResponse r1{};
    if (!stream->Read(&r1) || !r1.has_request_headers()) {
        return false;
    }

    // r2: response_headers — x-bytetaper-route-policy must match expected_policy
    envoy::service::ext_proc::v3::ProcessingResponse r2{};
    if (!stream->Read(&r2) || !r2.has_response_headers()) {
        return false;
    }

    for (const auto& mutation : r2.response_headers().response().header_mutation().set_headers()) {
        if (mutation.header().key() == "x-bytetaper-route-policy" &&
            mutation.header().raw_value() == expected_policy) {
            return true;
        }
    }
    return false;
}

int main() {
    bytetaper::policy::RoutePolicy policies[1] = {};
    policies[0].route_id = "api-v1";
    policies[0].match_kind = bytetaper::policy::RouteMatchKind::Prefix;
    policies[0].match_prefix = "/api/";

    bytetaper::extproc::GrpcServerConfig config{};
    config.policies = policies;
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

    // Stream 1: path matches prefix → expect "api-v1"
    if (!run_one_stream(channel, "/api/items", kExpectedMatch)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 100;
    }

    // Stream 2: path does not match → expect "none"
    if (!run_one_stream(channel, "/other/path", kExpectedNone)) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 101;
    }

    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
