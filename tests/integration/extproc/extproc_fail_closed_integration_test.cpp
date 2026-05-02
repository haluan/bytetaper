// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"
#include "extproc/grpc_server.h"
#include "policy/route_policy.h"

#include <chrono>
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <string>

// Malformed JSON that will trigger a safety error.
static constexpr const char* kMalformedBody = "{\"id\":1, \"unclosed_string: 2";

int main() {
    bytetaper::policy::RoutePolicy p1{};
    p1.route_id = "fail-closed-route";
    p1.match_prefix = "/api/fail";
    p1.match_kind = bytetaper::policy::RouteMatchKind::Prefix;
    p1.mutation = bytetaper::policy::MutationMode::Full;
    p1.failure_mode = bytetaper::policy::FailureMode::FailClosed;

    bytetaper::extproc::GrpcServerConfig config{};
    config.policies = &p1;
    config.policy_count = 1;
    config.listen_address = "127.0.0.1:0";

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
    grpc::ClientContext client_context{};
    auto stream = stub->Process(&client_context);

    // 1. request_headers: Match the fail-closed route
    {
        envoy::service::ext_proc::v3::ProcessingRequest req{};
        auto* hdrs = req.mutable_request_headers()->mutable_headers();
        auto* p = hdrs->add_headers();
        p->set_key(":path");
        p->set_raw_value("/api/fail/item?fields=id");
        stream->Write(req);

        envoy::service::ext_proc::v3::ProcessingResponse resp{};
        stream->Read(&resp);
    }

    // 2. response_headers: application/json
    {
        envoy::service::ext_proc::v3::ProcessingRequest req{};
        auto* hdrs = req.mutable_response_headers()->mutable_headers();
        auto* ct = hdrs->add_headers();
        ct->set_key("content-type");
        ct->set_raw_value("application/json");
        stream->Write(req);

        envoy::service::ext_proc::v3::ProcessingResponse resp{};
        stream->Read(&resp);
    }

    // 3. response_body: send malformed JSON
    {
        envoy::service::ext_proc::v3::ProcessingRequest req{};
        req.mutable_response_body()->set_body(kMalformedBody);
        req.mutable_response_body()->set_end_of_stream(true);
        stream->Write(req);

        envoy::service::ext_proc::v3::ProcessingResponse resp{};
        if (!stream->Read(&resp)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 5;
        }

        // Must have immediate_response because of fail-closed mode
        if (!resp.has_immediate_response()) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 100;
        }

        const auto& imm = resp.immediate_response();
        // Check for 500 status (Internal Server Error)
        if (imm.status().code() != envoy::type::v3::StatusCode::InternalServerError) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 101;
        }

        // Check for reason header
        bool found_reason = false;
        for (const auto& h : imm.headers().set_headers()) {
            if (h.header().key() == "x-bytetaper-fail-closed-reason" &&
                h.header().raw_value() == "invalid_json_safe_error") {
                found_reason = true;
                break;
            }
        }
        if (!found_reason) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 102;
        }
    }

    stream->WritesDone();
    stream->Finish();
    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
