#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Convert latency units (e.g., 2.5ms, 120us, 1s) to raw millisecond numbers
to_ms() {
    local val=$1
    if [[ "$val" =~ ([0-9.]+)(ms|us|s) ]]; then
        local num="${BASH_REMATCH[1]}"
        local unit="${BASH_REMATCH[2]}"
        if [ "$unit" = "us" ]; then
            awk "BEGIN {print $num / 1000}"
        elif [ "$unit" = "s" ]; then
            awk "BEGIN {print $num * 1000}"
        else
            echo "$num"
        fi
    else
        # Raw number assumed as ms
        if [[ "$val" =~ ^[0-9.]+$ ]]; then
            echo "$val"
        else
            echo ""
        fi
    fi
}

# Parse p50, p95, p99 latencies from wrk output file and return a structured JSON string
parse_latency() {
    local file=$1
    if [ ! -f "$file" ]; then
        echo "ERROR: wrk output file '$file' not found" >&2
        return 1
    fi

    # Try standard custom Lua reporter output line first
    local pct_line
    pct_line=$(grep -E "^LATENCY_PERCENTILES:" "$file" | tail -n 1 || true)

    local p50=""
    local p95=""
    local p99=""

    if [ -n "$pct_line" ]; then
        p50=$(echo "$pct_line" | sed -E 's/.*p50=([0-9.]+) ms.*/\1/' || true)
        p95=$(echo "$pct_line" | sed -E 's/.*p95=([0-9.]+) ms.*/\1/' || true)
        p99=$(echo "$pct_line" | sed -E 's/.*p99=([0-9.]+) ms.*/\1/' || true)
    else
        # Fallback to grepping default wrk output format if present
        local p50_raw
        local p99_raw
        p50_raw=$(grep -E "^[[:space:]]*50%" "$file" | awk '{print $2}' || true)
        p99_raw=$(grep -E "^[[:space:]]*99%" "$file" | awk '{print $2}' || true)

        if [ -n "$p50_raw" ]; then p50=$(to_ms "$p50_raw"); fi
        if [ -n "$p99_raw" ]; then p99=$(to_ms "$p99_raw"); fi
    fi

    # Strip any trailing/leading whitespace or non-numeric characters
    p50=$(echo "$p50" | xargs || true)
    p95=$(echo "$p95" | xargs || true)
    p99=$(echo "$p99" | xargs || true)

    # Validate all three values exist and are non-empty
    if [ -z "$p50" ] || [ -z "$p95" ] || [ -z "$p99" ]; then
        echo "ERROR: Missing required latency percentile data (p50='$p50', p95='$p95', p99='$p99') in file '$file'" >&2
        return 1
    fi

    # Build the valid JSON output conforming to behavior contract
    jq -c . <<EOF
{
  "latency_ms": {
    "p50": $p50,
    "p95": $p95,
    "p99": $p99
  }
}
EOF
}

# STANDALONE EXECUTION
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    if [ $# -ne 1 ]; then
        echo "Usage: $0 <wrk_output_file>" >&2
        exit 1
    fi
    parse_latency "$1"
fi
