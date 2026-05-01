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
