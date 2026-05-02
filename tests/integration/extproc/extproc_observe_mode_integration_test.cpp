// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"
#include "extproc/grpc_server.h"

#include <chrono>
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <string>

static constexpr const char* kInputBody = R"({"id": 1, "name": "Andi"})";

int main() {
    // Policy with mutation disabled (Observe Mode)
    bytetaper::policy::RoutePolicy p1{};
    p1.route_id = "observe-policy";
    p1.match_kind = bytetaper::policy::RouteMatchKind::Prefix;
    p1.match_prefix = "/api/";
    p1.mutation = bytetaper::policy::MutationMode::Disabled;

    bytetaper::extproc::GrpcServerConfig config{};
    config.policies = &p1;
    config.policy_count = 1;
    bytetaper::extproc::GrpcServerHandle handle{};
    if (!bytetaper::extproc::start_grpc_server(config, &handle)) {
        return 1;
    }

    const std::string target =
        "127.0.0.1:" + std::to_string(static_cast<std::uint32_t>(handle.bound_port));
    const auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto stub = envoy::service::ext_proc::v3::ExternalProcessor::NewStub(channel);
    grpc::ClientContext client_context{};
    auto stream = stub->Process(&client_context);

    // 1. request_headers
    {
        envoy::service::ext_proc::v3::ProcessingRequest req{};
        auto* hdrs = req.mutable_request_headers()->mutable_headers();
        auto* p = hdrs->add_headers();
        p->set_key(":path");
        p->set_raw_value("/api/data?fields=id");
        stream->Write(req);
        envoy::service::ext_proc::v3::ProcessingResponse resp{};
        stream->Read(&resp);
    }

    // 2. response_headers
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

    // 3. response_body
    {
        envoy::service::ext_proc::v3::ProcessingRequest req{};
        req.mutable_response_body()->set_body(kInputBody);
        req.mutable_response_body()->set_end_of_stream(true);
        stream->Write(req);

        envoy::service::ext_proc::v3::ProcessingResponse resp{};
        if (!stream->Read(&resp)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 5;
        }

        if (!resp.has_response_body()) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 100;
        }

        // Check for observe_mode reason header
        bool found_reason = false;
        for (const auto& h : resp.response_body().response().header_mutation().set_headers()) {
            if (h.header().key() == "x-bytetaper-fail-open-reason" &&
                h.header().raw_value() == "observe_mode") {
                found_reason = true;
                break;
            }
        }
        if (!found_reason) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 101;
        }

        // Verify body mutation is NOT present
        if (resp.response_body().response().has_body_mutation()) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 102;
        }
    }

    stream->WritesDone();
    stream->Finish();
    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
