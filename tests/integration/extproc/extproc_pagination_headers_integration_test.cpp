// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"
#include "extproc/grpc_server.h"
#include "policy/route_policy.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <grpcpp/grpcpp.h>
#include <string>

static bytetaper::policy::RoutePolicy make_pagination_policy() {
    bytetaper::policy::RoutePolicy p{};
    p.route_id = "test-pagination";
    p.match_kind = bytetaper::policy::RouteMatchKind::Prefix;
    p.match_prefix = "/orders";
    p.mutation = bytetaper::policy::MutationMode::Full;
    p.pagination.enabled = true;
    p.pagination.mode = bytetaper::policy::PaginationMode::LimitOffset;
    p.pagination.default_limit = 50;
    p.pagination.max_limit = 500;
    p.pagination.upstream_supports_pagination = true;
    std::strncpy(p.pagination.limit_param, "limit", 31);
    std::strncpy(p.pagination.offset_param, "offset", 31);
    return p;
}

static bool find_header(const envoy::service::ext_proc::v3::ProcessingResponse& resp,
                        const std::string& key, const std::string& expected_value) {
    if (!resp.has_response_headers())
        return false;
    for (const auto& m : resp.response_headers().response().header_mutation().set_headers()) {
        if (m.header().key() == key && m.header().raw_value() == expected_value) {
            return true;
        }
    }
    return false;
}

static bool header_absent(const envoy::service::ext_proc::v3::ProcessingResponse& resp,
                          const std::string& key) {
    if (!resp.has_response_headers())
        return true;
    for (const auto& m : resp.response_headers().response().header_mutation().set_headers()) {
        if (m.header().key() == key)
            return false;
    }
    return true;
}

int main() {
    auto policy = make_pagination_policy();

    bytetaper::extproc::GrpcServerConfig config{};
    config.policies = &policy;
    config.policy_count = 1;
    config.listen_address = "127.0.0.1:0";
    bytetaper::extproc::GrpcServerHandle handle{};

    if (!bytetaper::extproc::start_grpc_server(config, &handle))
        return 1;
    if (handle.bound_port == 0) {
        bytetaper::extproc::stop_grpc_server(&handle);
        return 2;
    }

    const std::string target = "127.0.0.1:" + std::to_string(handle.bound_port);
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

    // ── Scenario A: no limit ───────────────────────────────────────────
    {
        grpc::ClientContext ctx{};
        auto stream = stub->Process(&ctx);
        if (!stream) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 6;
        }

        envoy::service::ext_proc::v3::ProcessingRequest req_hdrs{};
        auto* rh = req_hdrs.mutable_request_headers()->mutable_headers()->add_headers();
        rh->set_key(":path");
        rh->set_raw_value("/orders");
        auto* rm = req_hdrs.mutable_request_headers()->mutable_headers()->add_headers();
        rm->set_key(":method");
        rm->set_raw_value("GET");
        if (!stream->Write(req_hdrs)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 7;
        }

        envoy::service::ext_proc::v3::ProcessingResponse r1{};
        if (!stream->Read(&r1)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 10;
        }

        envoy::service::ext_proc::v3::ProcessingRequest resp_hdrs{};
        auto* rs = resp_hdrs.mutable_response_headers()->mutable_headers()->add_headers();
        rs->set_key(":status");
        rs->set_raw_value("200");
        auto* ct = resp_hdrs.mutable_response_headers()->mutable_headers()->add_headers();
        ct->set_key("content-type");
        ct->set_raw_value("application/json");
        if (!stream->Write(resp_hdrs)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 8;
        }

        envoy::service::ext_proc::v3::ProcessingResponse r2{};
        if (!stream->Read(&r2)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 11;
        }

        // Check A results
        if (!find_header(r2, "x-bytetaper-pagination-applied", "true")) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 100;
        }
        if (!find_header(r2, "x-bytetaper-pagination-reason", "missing_limit")) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 101;
        }
        if (!find_header(r2, "x-bytetaper-pagination-limit", "50")) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 102;
        }

        envoy::service::ext_proc::v3::ProcessingRequest resp_body{};
        resp_body.mutable_response_body()->set_body("{}");
        resp_body.mutable_response_body()->set_end_of_stream(true);
        if (!stream->Write(resp_body)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 9;
        }

        envoy::service::ext_proc::v3::ProcessingResponse r3{};
        if (!stream->Read(&r3)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 12;
        }

        stream->WritesDone();
        stream->Finish();
    }

    // ── Scenario B: valid limit ────────────────────────────────────────
    {
        grpc::ClientContext ctx{};
        auto stream = stub->Process(&ctx);
        if (!stream) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 13;
        }

        envoy::service::ext_proc::v3::ProcessingRequest req_hdrs{};
        auto* rh = req_hdrs.mutable_request_headers()->mutable_headers()->add_headers();
        rh->set_key(":path");
        rh->set_raw_value("/orders?limit=100");
        auto* rm = req_hdrs.mutable_request_headers()->mutable_headers()->add_headers();
        rm->set_key(":method");
        rm->set_raw_value("GET");
        if (!stream->Write(req_hdrs)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 14;
        }

        envoy::service::ext_proc::v3::ProcessingResponse r1{};
        if (!stream->Read(&r1)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 17;
        }

        envoy::service::ext_proc::v3::ProcessingRequest resp_hdrs{};
        auto* rs = resp_hdrs.mutable_response_headers()->mutable_headers()->add_headers();
        rs->set_key(":status");
        rs->set_raw_value("200");
        auto* ct = resp_hdrs.mutable_response_headers()->mutable_headers()->add_headers();
        ct->set_key("content-type");
        ct->set_raw_value("application/json");
        if (!stream->Write(resp_hdrs)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 15;
        }

        envoy::service::ext_proc::v3::ProcessingResponse r2{};
        if (!stream->Read(&r2)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 18;
        }

        // Check B results
        if (!header_absent(r2, "x-bytetaper-pagination-applied")) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 103;
        }

        envoy::service::ext_proc::v3::ProcessingRequest resp_body{};
        resp_body.mutable_response_body()->set_body("{}");
        resp_body.mutable_response_body()->set_end_of_stream(true);
        if (!stream->Write(resp_body)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 16;
        }

        envoy::service::ext_proc::v3::ProcessingResponse r3{};
        if (!stream->Read(&r3)) {
            bytetaper::extproc::stop_grpc_server(&handle);
            return 19;
        }

        stream->WritesDone();
        stream->Finish();
    }

    bytetaper::extproc::stop_grpc_server(&handle);
    return 0;
}
