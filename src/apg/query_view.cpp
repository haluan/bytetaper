// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "apg/query_view.h"

#include <cstring>

namespace bytetaper::apg {

void parse_query_view(const char* raw_query, std::size_t raw_query_len, RequestQueryView* out) {
    if (out == nullptr) {
        return;
    }
    out->count = 0;
    if (raw_query == nullptr || raw_query_len == 0) {
        return;
    }

    const char* seg = raw_query;
    const char* end = raw_query + raw_query_len;

    while (seg < end && out->count < 32) {
        const char* amp =
            static_cast<const char*>(std::memchr(seg, '&', static_cast<std::size_t>(end - seg)));
        const char* seg_end = (amp != nullptr) ? amp : end;

        if (seg_end > seg) {
            const char* eq = static_cast<const char*>(
                std::memchr(seg, '=', static_cast<std::size_t>(seg_end - seg)));
            QueryParamView param{};

            if (eq != nullptr) {
                param.key = seg;
                std::size_t key_l = static_cast<std::size_t>(eq - seg);
                param.key_len = static_cast<std::uint16_t>(key_l > 0xFFFF ? 0xFFFF : key_l);

                param.value = eq + 1;
                std::size_t val_l = static_cast<std::size_t>(seg_end - (eq + 1));
                param.value_len = static_cast<std::uint16_t>(val_l > 0xFFFF ? 0xFFFF : val_l);
            } else {
                param.key = seg;
                std::size_t key_l = static_cast<std::size_t>(seg_end - seg);
                param.key_len = static_cast<std::uint16_t>(key_l > 0xFFFF ? 0xFFFF : key_l);
                param.value = nullptr;
                param.value_len = 0;
            }

            out->params[out->count] = param;
            out->count += 1;
        }

        seg = (amp != nullptr) ? amp + 1 : end;
    }
}

} // namespace bytetaper::apg
