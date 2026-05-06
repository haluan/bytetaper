#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Accept original and optimized bytes as arguments
ORIG_INPUT="${1:-}"
OPT_INPUT="${2:-}"

# Check for empty/missing input
if [ -z "$ORIG_INPUT" ] || [ -z "$OPT_INPUT" ] || [ "$ORIG_INPUT" = "unavailable" ] || [ "$OPT_INPUT" = "unavailable" ]; then
    jq -c -n \
        '{original_bytes_avg: "unavailable", optimized_bytes_avg: "unavailable", bytes_saved_avg: "unavailable", reduction_ratio: "unavailable"}'
    exit 0
fi

# Assert numeric format
if ! [[ "$ORIG_INPUT" =~ ^[0-9]+$ ]] || ! [[ "$OPT_INPUT" =~ ^[0-9]+$ ]]; then
    jq -c -n \
        '{original_bytes_avg: "unavailable", optimized_bytes_avg: "unavailable", bytes_saved_avg: "unavailable", reduction_ratio: "unavailable"}'
    exit 0
fi

ORIG_VAL=$((ORIG_INPUT))
OPT_VAL=$((OPT_INPUT))

# Calculate saved bytes
SAVED_VAL=$((ORIG_VAL - OPT_VAL))
if [ $SAVED_VAL -lt 0 ]; then
    SAVED_VAL=0
fi

# Calculate reduction ratio safely
RATIO_STR="0.00%"
if [ $ORIG_VAL -gt 0 ]; then
    # Calculate ratio using awk to support decimal precision
    RATIO_STR=$(awk "BEGIN {printf \"%.2f%%\", (${SAVED_VAL} / ${ORIG_VAL}) * 100}")
fi

# Output compact JSON
jq -c -n \
    --argjson orig "$ORIG_VAL" \
    --argjson opt "$OPT_VAL" \
    --argjson saved "$SAVED_VAL" \
    --arg ratio "$RATIO_STR" \
    '{"original_bytes_avg": $orig, "optimized_bytes_avg": $opt, "bytes_saved_avg": $saved, "reduction_ratio": $ratio}'
