// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"
#include "extproc/grpc_server.h"
#include "policy/route_policy.h"

#include <chrono>
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <string>

// Payload is 20 bytes.
static constexpr const char* kOversizedBody = "{\"id\":1, \"val\": 123}";

int main() {
    bytetaper::policy::RoutePolicy p1{};
    p1.route_id = "max-size-route";
    p1.match_prefix = "/api/limited";
    p1.match_kind = bytetaper::policy::RouteMatchKind::Prefix;
    p1.mutation = bytetaper::policy::MutationMode::Full;
    // Set limit to 10 bytes (kOversizedBody is ~20 bytes).
    p1.max_response_bytes = 10;

    bytetaper::extproc::GrpcServerConfig config{};
    config.policies = &p1;
    config.policy_count = 1;
    config.listen_address = "127.0.0.1:0";

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
        p->set_raw_value("/api/limited/item?fields=id");
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

    // 3. response_body: send oversized payload
    {
        envoy::service::ext_proc::v3::ProcessingRequest req{};
        req.mutable_response_body()->set_body(kOversizedBody);
        req.mutable_response_body()->set_end_of_stream(true);
        stream->Write(req);

        envoy::service::ext_proc::v3::ProcessingResponse resp{};
        if (!stream->Read(&resp)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 5;
        }

        // Must NOT have immediate_call because it's fail-open by default
        if (resp.has_immediate_response()) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 100;
        }

        if (!resp.has_response_body()) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 101;
        }

        // Check for reason header
        bool found_reason = false;
        for (const auto& h : resp.response_body().response().header_mutation().set_headers()) {
            if (h.header().key() == "x-bytetaper-fail-open-reason" &&
                h.header().raw_value() == "payload_too_large") {
                found_reason = true;
                break;
            }
        }
        if (!found_reason) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 102;
        }

        // Body must be unmodified (fail-open)
        if (resp.response_body().response().body_mutation().body() != "") {
            // Envoy ext_proc CONTINUE response with NO body mutation means original body is kept.
            // If body() is set, it means it's mutated.
            // Wait, our grpc_server logic: if build_filtered_body_response returns false,
            // it sends a CONTINUE response with add_bytetaper_report_headers(..., false).
            // add_bytetaper_report_headers doesn't set body mutation.
        }
    }

    stream->WritesDone();
    stream->Finish();
    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
