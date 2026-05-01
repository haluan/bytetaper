// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_EXTPROC_REQUEST_RUNTIME_H
#define BYTETAPER_EXTPROC_REQUEST_RUNTIME_H

#include <cstdint>

namespace bytetaper::extproc {

enum class ProcessingRequestKind : std::uint8_t {
    RequestHeaders = 0,
    ResponseHeaders = 1,
    ResponseBody = 2,
    Unsupported = 3,
};

struct ProcessingStreamStats {
    std::uint32_t total_messages = 0;
    std::uint32_t request_headers_count = 0;
    std::uint32_t response_headers_count = 0;
    std::uint32_t response_body_count = 0;
    std::uint32_t unsupported_count = 0;
};

ProcessingRequestKind classify_request_kind(bool has_request_headers, bool has_response_headers,
                                            bool has_response_body);
void record_request_kind(ProcessingRequestKind kind, ProcessingStreamStats* stats);

} // namespace bytetaper::extproc

#endif // BYTETAPER_EXTPROC_REQUEST_RUNTIME_H
