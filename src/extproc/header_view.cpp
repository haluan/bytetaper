// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/header_view.h"

#include "envoy/config/core/v3/base.pb.h"

#include <string>

namespace bytetaper::extproc {

RequestHeaderView scan_request_headers(const envoy::config::core::v3::HeaderMap& headers) {
    RequestHeaderView view{};
    for (const auto& header : headers.headers()) {
        const std::string& key = header.key();
        const std::string& val = header.raw_value().empty() ? header.value() : header.raw_value();

        if (key == ":path") {
            view.path = val.c_str();
            view.path_len = val.size();
        } else if (key == ":method") {
            view.method = val.c_str();
            view.method_len = val.size();
        } else if (key == "accept-encoding") {
            view.accept_encoding = val.c_str();
            view.accept_encoding_len = val.size();
        }
    }
    return view;
}

ResponseHeaderView scan_response_headers(const envoy::config::core::v3::HeaderMap& headers) {
    ResponseHeaderView view{};
    for (const auto& header : headers.headers()) {
        const std::string& key = header.key();
        const std::string& val = header.raw_value().empty() ? header.value() : header.raw_value();

        if (key == ":status") {
            view.status = val.c_str();
            view.status_len = val.size();
        } else if (key == "content-type") {
            view.content_type = val.c_str();
            view.content_type_len = val.size();
        } else if (key == "content-encoding") {
            view.content_encoding = val.c_str();
            view.content_encoding_len = val.size();
        } else if (key == "content-length") {
            view.content_length = val.c_str();
            view.content_length_len = val.size();
        }
    }
    return view;
}

} // namespace bytetaper::extproc
