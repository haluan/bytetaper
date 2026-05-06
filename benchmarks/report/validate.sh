#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

validate_report_file() {
    local file=$1
    if [ ! -f "$file" ]; then
        echo "ERROR: Report file '$file' not found" >&2
        return 1
    fi

    echo "Validating report file: $file"

    # Find all JSON latency blocks within the report file
    # We look for lines containing "Latency JSON" (e.g., "Latency JSON: { ... }", "Leg A Latency JSON: { ... }")
    local found=0
    while IFS= read -r line; do
        if [[ "$line" =~ Latency\ JSON:[[:space:]]*(.*) ]]; then
            found=1
            local json_str="${BASH_REMATCH[1]}"
            echo "Checking latency JSON: $json_str"

            # Parse using jq and assert fields exist
            local p50
            local p95
            local p99
            p50=$(echo "$json_str" | jq -e '.latency_ms.p50' 2>/dev/null || true)
            p95=$(echo "$json_str" | jq -e '.latency_ms.p95' 2>/dev/null || true)
            p99=$(echo "$json_str" | jq -e '.latency_ms.p99' 2>/dev/null || true)

            if [ -z "$p50" ] || [ "$p50" = "null" ]; then
                echo "ERROR: Missing 'latency_ms.p50' in report $file" >&2
                return 1
            fi
            if [ -z "$p95" ] || [ "$p95" = "null" ]; then
                echo "ERROR: Missing 'latency_ms.p95' in report $file" >&2
                return 1
            fi
            if [ -z "$p99" ] || [ "$p99" = "null" ]; then
                echo "ERROR: Missing 'latency_ms.p99' in report $file" >&2
                return 1
            fi

            echo "  - p50: $p50 ms"
            echo "  - p95: $p95 ms"
            echo "  - p99: $p99 ms"
        fi
    done < <(grep -E "Latency JSON" "$file" || true)

    if [ $found -eq 0 ]; then
        echo "ERROR: No latency JSON blocks found in report $file" >&2
        return 1
    fi

    echo "Report $file is fully VALID (all percentiles present)."
    return 0
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    if [ $# -ne 1 ]; then
        echo "Usage: $0 <report_file>" >&2
        exit 1
    fi
    validate_report_file "$1"
fi
