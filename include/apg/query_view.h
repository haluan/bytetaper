// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_QUERY_VIEW_H
#define BYTETAPER_APG_QUERY_VIEW_H

#include <cstddef>
#include <cstdint>

namespace bytetaper::apg {

struct QueryParamView {
    const char* key = nullptr;
    std::uint16_t key_len = 0;
    const char* value = nullptr;
    std::uint16_t value_len = 0;
};

struct RequestQueryView {
    QueryParamView params[32];
    std::uint8_t count = 0;
};

// Parses a raw query string into a bounded RequestQueryView.
// Parses up to 32 parameters, excess parameters are dropped deterministically (first 32 retained).
// No URL decoding is performed. No dynamic allocation is used.
void parse_query_view(const char* raw_query, std::size_t raw_query_len, RequestQueryView* out);

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_QUERY_VIEW_H
