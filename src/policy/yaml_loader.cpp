// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/yaml_loader.h"

#include <cstring>
#include <yaml-cpp/yaml.h>

namespace bytetaper::policy {

namespace {

// Parse a single route node into a PolicyFileResult slot. Returns false on error.
bool parse_one_route(const YAML::Node& node, PolicyFileResult* result, std::size_t index) {
    if (!node["id"] || !node["id"].IsScalar()) {
        result->error = "route missing required 'id' field";
        return false;
    }
    if (!node["match"] || !node["match"]["kind"] || !node["match"]["prefix"]) {
        result->error = "route missing required 'match.kind' or 'match.prefix'";
        return false;
    }

    const std::string id = node["id"].as<std::string>();
    const std::string kind = node["match"]["kind"].as<std::string>();
    const std::string prefix = node["match"]["prefix"].as<std::string>();
    const std::string mut = node["mutation"] ? node["mutation"].as<std::string>() : "disabled";
    const std::string method = node["method"] ? node["method"].as<std::string>() : "any";

    if (id.size() >= kMaxRouteIdLen) {
        result->error = "route_id exceeds max length";
        return false;
    }
    if (prefix.size() >= kMaxPrefixLen) {
        result->error = "match_prefix exceeds max length";
        return false;
    }

    std::memcpy(result->route_id_storage[index], id.c_str(), id.size() + 1);
    std::memcpy(result->match_prefix_storage[index], prefix.c_str(), prefix.size() + 1);

    RoutePolicy& policy = result->policies[index];
    policy.route_id = result->route_id_storage[index];
    policy.match_prefix = result->match_prefix_storage[index];

    if (kind == "prefix") {
        policy.match_kind = RouteMatchKind::Prefix;
    } else if (kind == "exact") {
        policy.match_kind = RouteMatchKind::Exact;
    } else {
        result->error = "unknown match.kind (expected 'prefix' or 'exact')";
        return false;
    }

    if (mut == "disabled") {
        policy.mutation = MutationMode::Disabled;
    } else if (mut == "headers_only") {
        policy.mutation = MutationMode::HeadersOnly;
    } else if (mut == "full") {
        policy.mutation = MutationMode::Full;
    } else {
        result->error = "unknown mutation (expected 'disabled', 'headers_only', or 'full')";
        return false;
    }

    if (method == "any") {
        policy.allowed_method = HttpMethod::Any;
    } else if (method == "get") {
        policy.allowed_method = HttpMethod::Get;
    } else if (method == "post") {
        policy.allowed_method = HttpMethod::Post;
    } else if (method == "put") {
        policy.allowed_method = HttpMethod::Put;
    } else if (method == "delete") {
        policy.allowed_method = HttpMethod::Delete;
    } else if (method == "patch") {
        policy.allowed_method = HttpMethod::Patch;
    } else {
        result->error =
            "unknown method (expected 'get', 'post', 'put', 'delete', 'patch', or 'any')";
        return false;
    }

    if (node["field_filter"]) {
        const YAML::Node& filter_node = node["field_filter"];
        if (!filter_node["mode"] || !filter_node["mode"].IsScalar()) {
            result->error = "field_filter missing 'mode'";
            return false;
        }

        const std::string mode_str = filter_node["mode"].as<std::string>();
        if (mode_str == "none") {
            policy.field_filter.mode = FieldFilterMode::None;
        } else if (mode_str == "allowlist") {
            policy.field_filter.mode = FieldFilterMode::Allowlist;
        } else if (mode_str == "denylist") {
            policy.field_filter.mode = FieldFilterMode::Denylist;
        } else {
            result->error =
                "unknown field_filter.mode (expected 'none', 'allowlist', or 'denylist')";
            return false;
        }

        if (policy.field_filter.mode != FieldFilterMode::None) {
            if (!filter_node["fields"] || !filter_node["fields"].IsSequence()) {
                result->error = "field_filter with mode must have 'fields' sequence";
                return false;
            }

            const YAML::Node& fields_node = filter_node["fields"];
            if (fields_node.size() > kMaxFields) {
                result->error = "too many field_filter fields (exceeds kMaxFields)";
                return false;
            }

            for (std::size_t i = 0; i < fields_node.size(); ++i) {
                if (!fields_node[i].IsScalar()) {
                    result->error = "field_filter field must be a scalar string";
                    return false;
                }
                const std::string f = fields_node[i].as<std::string>();
                if (f.empty()) {
                    result->error = "field_filter field name cannot be empty";
                    return false;
                }
                if (f.size() >= kMaxFieldNameLen) {
                    result->error = "field_filter field name exceeds max length";
                    return false;
                }
                std::memcpy(policy.field_filter.fields[i], f.c_str(), f.size() + 1);
            }
            policy.field_filter.field_count = fields_node.size();
        }
    }

    if (node["max_response_bytes"]) {
        if (!node["max_response_bytes"].IsScalar()) {
            result->error = "max_response_bytes must be a scalar integer";
            return false;
        }
        try {
            const std::uint64_t val = node["max_response_bytes"].as<std::uint64_t>();
            if (val > (std::uint64_t) 0xFFFFFFFFu) {
                result->error = "max_response_bytes exceeds uint32 max";
                return false;
            }
            policy.max_response_bytes = static_cast<std::uint32_t>(val);
        } catch (...) {
            result->error = "max_response_bytes must be a valid positive integer";
            return false;
        }
    }

    if (node["cache"]) {
        const YAML::Node& cache_node = node["cache"];
        if (cache_node["behavior"]) {
            if (!cache_node["behavior"].IsScalar()) {
                result->error = "cache.behavior must be a scalar string";
                return false;
            }
            const std::string behavior_str = cache_node["behavior"].as<std::string>();
            if (behavior_str == "default") {
                policy.cache.behavior = CacheBehavior::Default;
            } else if (behavior_str == "bypass") {
                policy.cache.behavior = CacheBehavior::Bypass;
            } else if (behavior_str == "store") {
                policy.cache.behavior = CacheBehavior::Store;
            } else {
                result->error = "unknown cache.behavior (expected 'default', 'bypass', or 'store')";
                return false;
            }
        }

        if (cache_node["ttl_seconds"]) {
            if (!cache_node["ttl_seconds"].IsScalar()) {
                result->error = "cache.ttl_seconds must be a scalar integer";
                return false;
            }
            try {
                policy.cache.ttl_seconds = cache_node["ttl_seconds"].as<std::uint32_t>();
            } catch (...) {
                result->error = "cache.ttl_seconds must be a valid positive integer";
                return false;
            }
        }
    }

    if (node["failure_mode"]) {
        if (!node["failure_mode"].IsScalar()) {
            result->error = "failure_mode must be a scalar string";
            return false;
        }
        const std::string fail_mode_str = node["failure_mode"].as<std::string>();
        if (fail_mode_str == "fail_open") {
            policy.failure_mode = FailureMode::FailOpen;
        } else if (fail_mode_str == "fail_closed") {
            policy.failure_mode = FailureMode::FailClosed;
        } else {
            result->error = "unknown failure_mode (expected 'fail_open' or 'fail_closed')";
            return false;
        }
    }
    if (node["pagination"]) {
        const YAML::Node& pag_node = node["pagination"];
        policy.pagination.enabled = pag_node["enabled"] ? pag_node["enabled"].as<bool>() : false;
        if (policy.pagination.enabled) {
            if (pag_node["mode"]) {
                const std::string mode_str = pag_node["mode"].as<std::string>();
                if (mode_str == "limit_offset") {
                    policy.pagination.mode = PaginationMode::LimitOffset;
                } else if (mode_str == "cursor") {
                    policy.pagination.mode = PaginationMode::Cursor;
                } else {
                    result->error = "unknown pagination.mode (expected 'limit_offset' or 'cursor')";
                    return false;
                }
            }
            if (pag_node["limit_param"]) {
                const std::string p = pag_node["limit_param"].as<std::string>();
                if (p.size() >= 32) {
                    result->error = "pagination.limit_param too long";
                    return false;
                }
                std::strncpy(policy.pagination.limit_param, p.c_str(), 31);
                policy.pagination.limit_param[31] = '\0';
            }
            if (pag_node["offset_param"]) {
                const std::string p = pag_node["offset_param"].as<std::string>();
                if (p.size() >= 32) {
                    result->error = "pagination.offset_param too long";
                    return false;
                }
                std::strncpy(policy.pagination.offset_param, p.c_str(), 31);
                policy.pagination.offset_param[31] = '\0';
            }
            if (pag_node["default_limit"]) {
                policy.pagination.default_limit = pag_node["default_limit"].as<std::uint32_t>();
            }
            if (pag_node["max_limit"]) {
                policy.pagination.max_limit = pag_node["max_limit"].as<std::uint32_t>();
            }
            if (pag_node["upstream_supports_pagination"]) {
                policy.pagination.upstream_supports_pagination =
                    pag_node["upstream_supports_pagination"].as<bool>();
            }
            if (pag_node["max_response_bytes_warning"]) {
                policy.pagination.max_response_bytes_warning =
                    pag_node["max_response_bytes_warning"].as<std::uint32_t>();
            }
        }
    }

    return true;
}

bool parse_document(const YAML::Node& doc, PolicyFileResult* result) {
    if (!doc["routes"] || !doc["routes"].IsSequence()) {
        result->error = "YAML document missing 'routes' sequence";
        return false;
    }

    const YAML::Node& routes = doc["routes"];
    if (routes.size() > kMaxRoutes) {
        result->error = "too many routes (exceeds kMaxRoutes)";
        return false;
    }

    for (std::size_t i = 0; i < routes.size(); ++i) {
        if (!parse_one_route(routes[i], result, i)) {
            return false;
        }
    }

    result->count = routes.size();
    result->ok = true;
    return true;
}

} // namespace

bool load_policy_from_string(const char* yaml_content, PolicyFileResult* result) {
    if (result == nullptr) {
        return false;
    }
    if (yaml_content == nullptr) {
        result->error = "yaml_content is null";
        return false;
    }
    try {
        const YAML::Node doc = YAML::Load(yaml_content);
        return parse_document(doc, result);
    } catch (const YAML::Exception& e) {
        result->error = "YAML parse error";
        return false;
    } catch (...) {
        result->error = "unknown error during YAML parse";
        return false;
    }
}

bool load_policy_from_file(const char* path, PolicyFileResult* result) {
    if (result == nullptr) {
        return false;
    }
    if (path == nullptr) {
        result->error = "path is null";
        return false;
    }
    try {
        const YAML::Node doc = YAML::LoadFile(path);
        return parse_document(doc, result);
    } catch (const YAML::Exception& e) {
        result->error = "YAML file load or parse error";
        return false;
    } catch (...) {
        result->error = "unknown error during YAML file load";
        return false;
    }
}

} // namespace bytetaper::policy
