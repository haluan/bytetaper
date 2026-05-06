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

echo "=== Running Payload Savings Parser Unit Tests ==="

# --------------------------------------------------
# Test Case 1: Mutation Scenario (savings occurred)
# --------------------------------------------------
JSON_MUTATION=$(./benchmarks/lib/payload_savings_parser.sh 10000 2000)

assert_equals "10000" "$(echo "$JSON_MUTATION" | jq -r '.original_bytes_avg')" "Mutation: original_bytes matches"
assert_equals "2000" "$(echo "$JSON_MUTATION" | jq -r '.optimized_bytes_avg')" "Mutation: optimized_bytes matches"
assert_equals "8000" "$(echo "$JSON_MUTATION" | jq -r '.bytes_saved_avg')" "Mutation: bytes_saved calculated correctly"
assert_equals "80.00%" "$(echo "$JSON_MUTATION" | jq -r '.reduction_ratio')" "Mutation: reduction ratio matches exactly"

# --------------------------------------------------
# Test Case 2: No-Mutation Scenario (original equals optimized)
# --------------------------------------------------
JSON_NOMUT=$(./benchmarks/lib/payload_savings_parser.sh 5000 5000)

assert_equals "5000" "$(echo "$JSON_NOMUT" | jq -r '.original_bytes_avg')" "No-Mutation: original_bytes matches"
assert_equals "5000" "$(echo "$JSON_NOMUT" | jq -r '.optimized_bytes_avg')" "No-Mutation: optimized_bytes matches"
assert_equals "0" "$(echo "$JSON_NOMUT" | jq -r '.bytes_saved_avg')" "No-Mutation: bytes_saved is 0"
assert_equals "0.00%" "$(echo "$JSON_NOMUT" | jq -r '.reduction_ratio')" "No-Mutation: reduction ratio is 0.00%"

# --------------------------------------------------
# Test Case 3: Division-by-Zero Safety
# --------------------------------------------------
JSON_ZERO=$(./benchmarks/lib/payload_savings_parser.sh 0 0)

assert_equals "0" "$(echo "$JSON_ZERO" | jq -r '.original_bytes_avg')" "Zero: original_bytes is 0"
assert_equals "0" "$(echo "$JSON_ZERO" | jq -r '.optimized_bytes_avg')" "Zero: optimized_bytes is 0"
assert_equals "0" "$(echo "$JSON_ZERO" | jq -r '.bytes_saved_avg')" "Zero: bytes_saved is 0"
assert_equals "0.00%" "$(echo "$JSON_ZERO" | jq -r '.reduction_ratio')" "Zero: reduction ratio is 0.00% without crashing"

# --------------------------------------------------
# Test Case 4: Missing or Unavailable Inputs
# --------------------------------------------------
JSON_UNAVAIL1=$(./benchmarks/lib/payload_savings_parser.sh)
JSON_UNAVAIL2=$(./benchmarks/lib/payload_savings_parser.sh "unavailable" 123)
JSON_UNAVAIL3=$(./benchmarks/lib/payload_savings_parser.sh "abc" "xyz")

assert_equals "unavailable" "$(echo "$JSON_UNAVAIL1" | jq -r '.original_bytes_avg')" "Missing inputs: original_bytes_avg is unavailable"
assert_equals "unavailable" "$(echo "$JSON_UNAVAIL1" | jq -r '.reduction_ratio')" "Missing inputs: reduction_ratio is unavailable"
assert_equals "unavailable" "$(echo "$JSON_UNAVAIL2" | jq -r '.bytes_saved_avg')" "Explicit unavailable input: bytes_saved_avg is unavailable"
assert_equals "unavailable" "$(echo "$JSON_UNAVAIL3" | jq -r '.optimized_bytes_avg')" "Invalid string input: optimized_bytes_avg is unavailable"

echo "=== All Payload Savings Parser Unit Tests Passed ==="
exit 0
