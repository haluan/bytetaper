// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "policy/route_policy.h"
#include "policy/yaml_loader.h"
#include <cstdio>
#include <cstring>

/**
 * bytetaper-validate-policy CLI tool.
 *
 * Usage: bytetaper-validate-policy <path-to-policy.yaml>
 *
 * Exit codes:
 * 0 - Success: all routes valid
 * 1 - Usage error: wrong argument count
 * 2 - Load/Parse error: YAML file invalid or missing
 * 3 - Validation error: one or more routes failed validation
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: bytetaper-validate-policy <policy.yaml>\n");
        return 1;
    }

    bytetaper::policy::PolicyFileResult result{};
    if (!bytetaper::policy::load_policy_from_file(argv[1], &result)) {
        std::fprintf(stderr, "error: %s\n", result.error ? result.error : "unknown");
        return 2;
    }

    for (std::size_t i = 0; i < result.count; ++i) {
        const char* reason = nullptr;
        if (!bytetaper::policy::validate_route_policy(result.policies[i], &reason)) {
            std::fprintf(stderr, "invalid route '%s': %s\n",
                         result.policies[i].route_id ? result.policies[i].route_id : "(null)",
                         reason ? reason : "unknown");
            return 3;
        }
    }

    std::printf("OK: %zu route(s) validated\n", result.count);
    return 0;
}
