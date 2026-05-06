#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

parse_throughput() {
    local file=$1
    if [ ! -f "$file" ]; then
        echo "ERROR: wrk output file '$file' not found" >&2
        return 1
    fi

    # Parse Requests/sec (RPS)
    local rps
    rps=$(grep -E "^[[:space:]]*Requests/sec:" "$file" | awk '{print $2}' || true)
    rps=$(echo "$rps" | xargs || true)

    # Parse Total Requests
    local total_reqs
    total_reqs=$(grep -E '^[[:space:]]*[0-9]+ requests in' "$file" | awk '{print $1}' || true)
    total_reqs=$(echo "$total_reqs" | xargs || true)

    if [ -z "$rps" ] || [ -z "$total_reqs" ]; then
        echo "ERROR: Failed to parse required throughput metrics (rps='$rps', total_reqs='$total_reqs') in file '$file'" >&2
        return 1
    fi

    # Parse non-2xx/3xx if present
    local non_2xx
    non_2xx=$(grep -E "Non-2xx or 3xx responses:" "$file" | awk '{print $5}' || echo "0")
    non_2xx=$(echo "$non_2xx" | xargs || echo "0")
    if [ -z "$non_2xx" ] || ! [[ "$non_2xx" =~ ^[0-9]+$ ]]; then
        non_2xx=0
    fi

    # Parse socket errors if present
    # Line format: Socket errors: connect 0, read 2, write 0, timeout 5
    local socket_line
    socket_line=$(grep -E "^[[:space:]]*Socket errors:" "$file" | tail -n 1 || true)
    local socket_errs=0
    if [ -n "$socket_line" ]; then
        local c r w t
        c=$(echo "$socket_line" | sed -E 's/.*connect ([0-9]+).*/\1/' || echo "0")
        r=$(echo "$socket_line" | sed -E 's/.*read ([0-9]+).*/\1/' || echo "0")
        w=$(echo "$socket_line" | sed -E 's/.*write ([0-9]+).*/\1/' || echo "0")
        t=$(echo "$socket_line" | sed -E 's/.*timeout ([0-9]+).*/\1/' || echo "0")
        # Sum them up safely using awk
        socket_errs=$(awk "BEGIN {print $c + $r + $w + $t}" 2>/dev/null || echo "0")
    fi
    if [ -z "$socket_errs" ] || ! [[ "$socket_errs" =~ ^[0-9]+$ ]]; then
        socket_errs=0
    fi

    # Compute failed and successful requests
    local failed_reqs=$((non_2xx + socket_errs))
    local successful_reqs=$((total_reqs - failed_reqs))
    if [ "$successful_reqs" -lt 0 ]; then
        successful_reqs=0
    fi

    # Build the valid JSON output conforming to the behavior contract
    jq -c . <<EOF
{
  "throughput": {
    "requests_per_second": $rps,
    "total_requests": $total_reqs,
    "successful_requests": $successful_reqs,
    "failed_requests": $failed_reqs
  },
  "total_requests": $total_reqs,
  "successful_requests": $successful_reqs,
  "failed_requests": $failed_reqs
}
EOF
}

# STANDALONE EXECUTION
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    if [ $# -ne 1 ]; then
        echo "Usage: $0 <wrk_output_file>" >&2
        exit 1
    fi
    parse_throughput "$1"
fi
