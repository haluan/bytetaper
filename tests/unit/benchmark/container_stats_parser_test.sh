#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

assert_equals() {
    local expected=$1
    local actual=$2
    local msg=$3
    if [ "$expected" != "$actual" ]; then
        echo "FAIL: $msg" >&2
        echo "  Expected: '$expected'" >&2
        echo "  Actual:   '$actual'" >&2
        exit 1
    else
        echo "PASS: $msg"
    fi
}

echo "=== Running Container Stats Unit Tests ==="

# --------------------------------------------------
# Test Case 1: Default values (should be unavailable)
# --------------------------------------------------
JSON_DEFAULT=$(./benchmarks/lib/container_stats.sh all)

assert_equals "unavailable" "$(echo "$JSON_DEFAULT" | jq -r '.envoy.cpu_percent')" "Default: envoy CPU is unavailable"
assert_equals "unavailable" "$(echo "$JSON_DEFAULT" | jq -r '.envoy.peak_memory_mb')" "Default: envoy memory is unavailable"
assert_equals "unavailable" "$(echo "$JSON_DEFAULT" | jq -r '."bytetaper-extproc".cpu_percent')" "Default: extproc CPU is unavailable"
assert_equals "unavailable" "$(echo "$JSON_DEFAULT" | jq -r '."mock-api".peak_memory_mb')" "Default: mock-api memory is unavailable"

# --------------------------------------------------
# Test Case 2: Override environment variables
# --------------------------------------------------
export OVERRIDE_ENVOY_CPU="12.5"
export OVERRIDE_ENVOY_MEM="48.2"
export OVERRIDE_EXTPROC_CPU="5"
export OVERRIDE_EXTPROC_MEM="32"
export OVERRIDE_MOCK_CPU="1.2"
export OVERRIDE_MOCK_MEM="15.5"

JSON_OVERRIDE=$(./benchmarks/lib/container_stats.sh all)

assert_equals "12.5" "$(echo "$JSON_OVERRIDE" | jq -r '.envoy.cpu_percent')" "Override: envoy CPU matches"
assert_equals "48.2" "$(echo "$JSON_OVERRIDE" | jq -r '.envoy.peak_memory_mb')" "Override: envoy memory matches"
assert_equals "5" "$(echo "$JSON_OVERRIDE" | jq -r '."bytetaper-extproc".cpu_percent')" "Override: extproc CPU matches numeric conversion"
assert_equals "32" "$(echo "$JSON_OVERRIDE" | jq -r '."bytetaper-extproc".peak_memory_mb')" "Override: extproc memory matches numeric conversion"

# --------------------------------------------------
# Test Case 3: Target-specific query
# --------------------------------------------------
JSON_ENVOY=$(./benchmarks/lib/container_stats.sh envoy)
assert_equals "12.5" "$(echo "$JSON_ENVOY" | jq -r '.cpu_percent')" "Target query: envoy CPU"
assert_equals "48.2" "$(echo "$JSON_ENVOY" | jq -r '.peak_memory_mb')" "Target query: envoy memory"

# Reset overrides
unset OVERRIDE_ENVOY_CPU OVERRIDE_ENVOY_MEM OVERRIDE_EXTPROC_CPU OVERRIDE_EXTPROC_MEM OVERRIDE_MOCK_CPU OVERRIDE_MOCK_MEM

echo "=== All Container Stats Unit Tests Passed ==="
exit 0
