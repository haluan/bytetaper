#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Get stat for a given service, incorporating dummy override for testability
get_stats() {
    local service=$1

    # Default values are "unavailable" conforming to contract
    local cpu="unavailable"
    local mem="unavailable"

    # Environment variable overrides for testing/simulation
    case "$service" in
        envoy)
            if [ -n "${OVERRIDE_ENVOY_CPU:-}" ]; then cpu="${OVERRIDE_ENVOY_CPU}"; fi
            if [ -n "${OVERRIDE_ENVOY_MEM:-}" ]; then mem="${OVERRIDE_ENVOY_MEM}"; fi
            ;;
        bytetaper-extproc)
            if [ -n "${OVERRIDE_EXTPROC_CPU:-}" ]; then cpu="${OVERRIDE_EXTPROC_CPU}"; fi
            if [ -n "${OVERRIDE_EXTPROC_MEM:-}" ]; then mem="${OVERRIDE_EXTPROC_MEM}"; fi
            ;;
        mock-api)
            if [ -n "${OVERRIDE_MOCK_CPU:-}" ]; then cpu="${OVERRIDE_MOCK_CPU}"; fi
            if [ -n "${OVERRIDE_MOCK_MEM:-}" ]; then mem="${OVERRIDE_MOCK_MEM}"; fi
            ;;
    esac

    # Build and compact JSON
    jq -c -n \
        --arg cpu "$cpu" \
        --arg mem "$mem" \
        '{cpu_percent: (if $cpu == "unavailable" then "unavailable" else (try ($cpu | tonumber) catch $cpu) end), peak_memory_mb: (if $mem == "unavailable" then "unavailable" else (try ($mem | tonumber) catch $mem) end)}'
}

# Standalone run logic
if [ $# -eq 0 ] || [ "$1" = "all" ]; then
    # Return all 3 services in a consolidated schema
    jq -c -n \
        --argjson envoy "$(get_stats envoy)" \
        --argjson extproc "$(get_stats bytetaper-extproc)" \
        --argjson mock "$(get_stats mock-api)" \
        '{"envoy": $envoy, "bytetaper-extproc": $extproc, "mock-api": $mock}'
elif [ $# -eq 1 ]; then
    get_stats "$1"
else
    echo "Usage: $0 [envoy | bytetaper-extproc | mock-api | all]" >&2
    exit 1
fi
