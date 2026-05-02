// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#ifndef BYTETAPER_APG_CONTEXT_H
#define BYTETAPER_APG_CONTEXT_H

#include "cache/cache_entry.h"
#include "cache/l1_cache.h"
#include "policy/field_filter_policy.h"
#include "policy/route_policy.h"

#include <cstddef>
#include <cstdint>

namespace bytetaper::apg {

struct ApgTransformContext {
    static constexpr std::size_t kRawPathBufferSize = 1024;
    static constexpr std::size_t kRawQueryBufferSize = 1024;

    std::uint64_t request_id = 0;
    std::size_t input_payload_bytes = 0;
    std::size_t output_payload_bytes = 0;
    std::uint32_t executed_stage_count = 0;
    char raw_path[kRawPathBufferSize] = {};
    std::size_t raw_path_length = 0;
    char raw_query[kRawQueryBufferSize] = {};
    std::size_t raw_query_length = 0;
    char selected_fields[policy::kMaxFields][policy::kMaxFieldNameLen] = {};
    // Canonical API-intelligence metric.
    // This count represents selected fields after policy filtering.
    std::size_t selected_field_count = 0;
    // Canonical API-intelligence metric for JSON transform filtering.
    // This count represents object field keys removed by filtering in the
    // latest transform call.
    mutable std::size_t removed_field_count = 0;
    char* trace_buffer = nullptr;
    std::size_t trace_capacity = 0;
    std::size_t trace_length = 0;

    // --- Cache lookup inputs (set by caller before running pipeline) ---
    const policy::RoutePolicy* matched_policy = nullptr;
    cache::L1Cache* l1_cache = nullptr;
    std::int64_t request_epoch_ms = 0;
    policy::HttpMethod request_method = policy::HttpMethod::Get;

    // --- Cache lookup outputs (written by l1_cache_lookup_stage) ---
    bool cache_hit = false;
    const char* cache_layer = nullptr; // "L1" on hit, nullptr on miss
    bool should_return_immediate_response = false;
    cache::CacheEntry cached_response{}; // populated on hit; body is non-owning

    // --- Cache store inputs (set by caller after receiving upstream response) ---
    std::uint16_t response_status_code = 0;
    const char* response_body = nullptr; // non-owning
    std::size_t response_body_len = 0;
    char response_content_type[cache::kCacheContentTypeMaxLen] = {};
};

} // namespace bytetaper::apg

#endif // BYTETAPER_APG_CONTEXT_H
