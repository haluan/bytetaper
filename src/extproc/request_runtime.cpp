// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "extproc/request_runtime.h"

namespace bytetaper::extproc {

ProcessingRequestKind classify_request_kind(bool has_request_headers, bool has_response_headers,
                                            bool has_response_body) {
    if (has_request_headers) {
        return ProcessingRequestKind::RequestHeaders;
    }
    (void) has_response_headers;
    (void) has_response_body;
    return ProcessingRequestKind::Unsupported;
}

void record_request_kind(ProcessingRequestKind kind, ProcessingStreamStats* stats) {
    if (stats == nullptr) {
        return;
    }

    stats->total_messages += 1;
    if (kind == ProcessingRequestKind::RequestHeaders) {
        stats->request_headers_count += 1;
        return;
    }
    stats->unsupported_count += 1;
}

} // namespace bytetaper::extproc
