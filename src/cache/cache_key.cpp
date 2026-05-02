// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_key.h"

#include <cstdio>
#include <cstring>

namespace bytetaper::cache {

namespace {

bool key_append(char** pos, std::size_t* remaining, const char* s, std::size_t len) {
    if (len > *remaining) {
        return false;
    }
    std::memcpy(*pos, s, len);
    *pos += len;
    *remaining -= len;
    return true;
}

bool key_append_char(char** pos, std::size_t* remaining, char c) {
    if (*remaining == 0) {
        return false;
    }
    **pos = c;
    ++(*pos);
    --(*remaining);
    return true;
}

} // namespace

bool build_cache_key(const CacheKeyInput& input, char* out_buf, std::size_t out_size) {
    if (input.method != policy::HttpMethod::Get) {
        return false;
    }
    if (input.route_id == nullptr || input.path == nullptr) {
        return false;
    }
    if (out_buf == nullptr || out_size == 0) {
        return false;
    }

    // Copy and sort selected_fields pointers (selection sort, max 16 elements).
    const std::size_t count = input.selected_field_count < policy::kMaxFields
                                  ? input.selected_field_count
                                  : policy::kMaxFields;
    const char* sorted[policy::kMaxFields];
    for (std::size_t i = 0; i < count; ++i) {
        sorted[i] = (input.selected_fields != nullptr) ? input.selected_fields[i] : "";
    }
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t min_idx = i;
        for (std::size_t j = i + 1; j < count; ++j) {
            if (std::strncmp(sorted[j], sorted[min_idx], policy::kMaxFieldNameLen) < 0) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            const char* tmp = sorted[i];
            sorted[i] = sorted[min_idx];
            sorted[min_idx] = tmp;
        }
    }

    char* pos = out_buf;
    std::size_t remaining = out_size - 1;

    // Format: GET|{route_id}|{path}|{query}|{f1},{f2}|{policy_version}
    if (!key_append(&pos, &remaining, "GET|", 4)) {
        return false;
    }
    const std::size_t route_id_len = std::strlen(input.route_id);
    if (!key_append(&pos, &remaining, input.route_id, route_id_len)) {
        return false;
    }
    if (!key_append_char(&pos, &remaining, '|')) {
        return false;
    }
    const std::size_t path_len = std::strlen(input.path);
    if (!key_append(&pos, &remaining, input.path, path_len)) {
        return false;
    }
    if (!key_append_char(&pos, &remaining, '|')) {
        return false;
    }
    if (input.query != nullptr && input.query[0] != '\0') {
        const std::size_t query_len = std::strlen(input.query);
        if (!key_append(&pos, &remaining, input.query, query_len)) {
            return false;
        }
    }
    if (!key_append_char(&pos, &remaining, '|')) {
        return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            if (!key_append_char(&pos, &remaining, ',')) {
                return false;
            }
        }
        const std::size_t flen = std::strlen(sorted[i]);
        if (!key_append(&pos, &remaining, sorted[i], flen)) {
            return false;
        }
    }
    if (!key_append_char(&pos, &remaining, '|')) {
        return false;
    }
    if (input.policy_version != nullptr && input.policy_version[0] != '\0') {
        const std::size_t ver_len = std::strlen(input.policy_version);
        if (!key_append(&pos, &remaining, input.policy_version, ver_len)) {
            return false;
        }
    }

    if (input.private_cache) {
        if (input.auth_scope == nullptr || input.auth_scope[0] == '\0') {
            return false; // missing scope rejects private cache
        }
        // append |scope:{auth_scope}
        int n = std::snprintf(pos, remaining, "|scope:%s", input.auth_scope);
        if (n < 0 || static_cast<std::size_t>(n) >= remaining) {
            return false;
        }
        pos += n;
        remaining -= static_cast<std::size_t>(n);
    }

    *pos = '\0';
    return true;
}

} // namespace bytetaper::cache
