// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/grpc_server.h"
#include "extproc/request_runtime.h"
#include "field_selection/request_target.h"
#include "json_transform/content_type.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "envoy/service/ext_proc/v3/external_processor.grpc.pb.h"

namespace bytetaper::extproc {

namespace {

constexpr const char* kPathHeader = ":path";
constexpr const char* kContentTypeHeader = "content-type";
constexpr const char* kContentLengthHeader = "content-length";
constexpr const char* kResponseBodyHeader = "x-bytetaper-extproc-response-body";
constexpr const char* kOriginalResponseBytesHeader = "x-bytetaper-original-response-bytes";
constexpr const char* kWasteRemovedFieldsHeader = "x-bytetaper-waste-removed-fields";
constexpr const char* kWasteSavedBytesHeader = "x-bytetaper-waste-saved-bytes";
constexpr const char* kOptimizedResponseBytesHeader = "x-bytetaper-optimized-response-bytes";
constexpr const char* kOriginalBytesHeader = "x-bytetaper-original-bytes";
constexpr const char* kOptimizedBytesHeader = "x-bytetaper-optimized-bytes";

constexpr const char* kTrueValue = "true";

struct StreamFilterState {
    apg::ApgTransformContext context{};
    json_transform::JsonResponseKind response_kind =
        json_transform::JsonResponseKind::SkipUnsupported;
    bool has_query_selection = false;
};

void add_overwrite_header(envoy::service::ext_proc::v3::CommonResponse* common, const char* key,
                          const std::string& value) {
    if (common == nullptr || key == nullptr) {
        return;
    }
    auto* mutation = common->mutable_header_mutation()->add_set_headers();
    mutation->mutable_header()->set_key(key);
    mutation->mutable_header()->set_raw_value(value);
    mutation->set_append_action(
        envoy::config::core::v3::HeaderValueOption::OVERWRITE_IF_EXISTS_OR_ADD);
}

void add_waste_report_headers(envoy::service::ext_proc::v3::CommonResponse* common,
                              std::size_t removed_fields, std::size_t saved_bytes) {
    add_overwrite_header(common, kWasteRemovedFieldsHeader, std::to_string(removed_fields));
    add_overwrite_header(common, kWasteSavedBytesHeader, std::to_string(saved_bytes));
}

bool read_header_value(const envoy::config::core::v3::HeaderMap& headers, const char* key,
                       std::string* value_out) {
    if (key == nullptr || value_out == nullptr) {
        return false;
    }

    for (const auto& header : headers.headers()) {
        if (header.key() != key) {
            continue;
        }
        if (!header.raw_value().empty()) {
            *value_out = header.raw_value();
            return true;
        }
        *value_out = header.value();
        return true;
    }

    return false;
}

void apply_request_headers_selection(const envoy::service::ext_proc::v3::ProcessingRequest& request,
                                     StreamFilterState* state) {
    if (state == nullptr || !request.has_request_headers()) {
        return;
    }

    state->context = apg::ApgTransformContext{};
    state->response_kind = json_transform::JsonResponseKind::SkipUnsupported;
    state->has_query_selection = false;

    std::string request_path{};
    if (!read_header_value(request.request_headers().headers(), kPathHeader, &request_path)) {
        return;
    }

    if (!field_selection::extract_raw_path_and_query(request_path.c_str(), &state->context)) {
        return;
    }
    if (!field_selection::parse_and_store_selected_fields(&state->context)) {
        return;
    }

    state->has_query_selection = state->context.selected_field_count > 0;
}

void apply_response_content_type(const envoy::service::ext_proc::v3::ProcessingRequest& request,
                                 StreamFilterState* state) {
    if (state == nullptr || !request.has_response_headers()) {
        return;
    }

    std::string content_type{};
    if (!read_header_value(request.response_headers().headers(), kContentTypeHeader,
                           &content_type)) {
        state->response_kind = json_transform::JsonResponseKind::SkipUnsupported;
        return;
    }

    json_transform::JsonResponseKind detected = json_transform::JsonResponseKind::SkipUnsupported;
    if (!json_transform::detect_application_json_response(content_type.c_str(), &detected)) {
        state->response_kind = json_transform::JsonResponseKind::SkipUnsupported;
        return;
    }
    state->response_kind = detected;
}

bool build_filtered_body_response(const envoy::service::ext_proc::v3::ProcessingRequest& request,
                                  StreamFilterState& state,
                                  envoy::service::ext_proc::v3::ProcessingResponse* response_out) {
    if (response_out == nullptr || !request.has_response_body()) {
        return false;
    }
    if (!state.has_query_selection) {
        return false;
    }
    if (state.response_kind != json_transform::JsonResponseKind::EligibleJson) {
        return false;
    }
    if (!request.response_body().end_of_stream()) {
        return false;
    }

    const std::string input_body = request.response_body().body();
    state.context.input_payload_bytes = input_body.size();
    json_transform::ParsedFlatJsonObject parsed{};
    const json_transform::FlatJsonParseStatus parse_status =
        json_transform::parse_flat_json_object(input_body.c_str(), state.response_kind, &parsed);

    std::vector<char> output(input_body.size() + 1, '\0');
    std::size_t output_length = 0;
    const json_transform::FlatJsonFilterStatus status =
        json_transform::transform_flat_json_with_filter_toggle(
            input_body.c_str(), parse_status, &parsed, state.context, true, output.data(),
            output.size(), &output_length);
    if (status != json_transform::FlatJsonFilterStatus::Ok) {
        return false;
    }

    std::string filtered_body(output.data(), output_length);
    const std::size_t saved_bytes = (input_body.size() >= filtered_body.size())
                                        ? (input_body.size() - filtered_body.size())
                                        : 0;
    auto* body_response = response_out->mutable_response_body();
    auto* common = body_response->mutable_response();
    common->set_status(envoy::service::ext_proc::v3::CommonResponse::CONTINUE);
    common->mutable_body_mutation()->set_body(filtered_body);

    add_overwrite_header(common, kResponseBodyHeader, kTrueValue);
    add_overwrite_header(common, kContentLengthHeader, std::to_string(filtered_body.size()));
    add_waste_report_headers(common, state.context.removed_field_count, saved_bytes);
    add_overwrite_header(common, kOriginalResponseBytesHeader, std::to_string(input_body.size()));
    add_overwrite_header(common, kOriginalBytesHeader, std::to_string(input_body.size()));
    state.context.output_payload_bytes = filtered_body.size();
    add_overwrite_header(common, kOptimizedResponseBytesHeader,
                         std::to_string(filtered_body.size()));
    add_overwrite_header(common, kOptimizedBytesHeader, std::to_string(filtered_body.size()));

    return true;
}

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
        StreamFilterState filter_state{};
        envoy::service::ext_proc::v3::ProcessingRequest request{};
        while (stream->Read(&request)) {
            const ProcessingRequestKind kind =
                classify_request_kind(request.has_request_headers(), request.has_response_headers(),
                                      request.has_response_body());
            record_request_kind(kind, &stream_stats);

            if (kind == ProcessingRequestKind::RequestHeaders) {
                apply_request_headers_selection(request, &filter_state);
                envoy::service::ext_proc::v3::ProcessingResponse response{};
                auto* request_headers_response = response.mutable_request_headers();
                request_headers_response->mutable_response()->set_status(
                    envoy::service::ext_proc::v3::CommonResponse::CONTINUE);
                stream->Write(response);
                continue;
            }
            if (kind == ProcessingRequestKind::ResponseHeaders) {
                apply_response_content_type(request, &filter_state);
                envoy::service::ext_proc::v3::ProcessingResponse response{};
                auto* response_headers = response.mutable_response_headers();
                auto* common_response = response_headers->mutable_response();
                common_response->set_status(envoy::service::ext_proc::v3::CommonResponse::CONTINUE);
                add_overwrite_header(common_response, kResponseBodyHeader, kTrueValue);
                add_waste_report_headers(common_response, 0, 0);
                add_overwrite_header(common_response, kOriginalBytesHeader, "0");
                add_overwrite_header(common_response, kOptimizedBytesHeader, "0");
                stream->Write(response);
                continue;
            }
            if (kind == ProcessingRequestKind::ResponseBody) {
                envoy::service::ext_proc::v3::ProcessingResponse response{};
                if (!build_filtered_body_response(request, filter_state, &response)) {
                    auto* response_body = response.mutable_response_body();
                    auto* common_response = response_body->mutable_response();
                    common_response->set_status(
                        envoy::service::ext_proc::v3::CommonResponse::CONTINUE);
                    add_overwrite_header(common_response, kResponseBodyHeader, kTrueValue);
                    add_waste_report_headers(common_response, 0, 0);
                    filter_state.context.input_payload_bytes =
                        request.response_body().body().size();
                    add_overwrite_header(common_response, kOriginalResponseBytesHeader,
                                         std::to_string(request.response_body().body().size()));
                    add_overwrite_header(common_response, kOriginalBytesHeader,
                                         std::to_string(request.response_body().body().size()));
                    add_overwrite_header(common_response, kOptimizedBytesHeader,
                                         std::to_string(request.response_body().body().size()));
                    filter_state.context.output_payload_bytes =
                        request.response_body().body().size();
                    add_overwrite_header(common_response, kOptimizedResponseBytesHeader,
                                         std::to_string(request.response_body().body().size()));
                }
                stream->Write(response);
                continue;
            }
            // Non-supported variants are safe no-op.
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
