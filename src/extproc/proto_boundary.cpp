// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/proto_boundary.h"

#include "envoy/service/ext_proc/v3/external_processor.pb.h"

namespace bytetaper::extproc {

bool verify_proto_linkage() {
    envoy::service::ext_proc::v3::ProcessingRequest request{};
    request.mutable_request_headers();

    envoy::service::ext_proc::v3::ProcessingResponse response{};
    response.mutable_request_headers();

    return request.has_request_headers() && response.has_request_headers();
}

} // namespace bytetaper::extproc
