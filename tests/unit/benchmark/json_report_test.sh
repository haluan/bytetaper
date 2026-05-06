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

echo "=== Running JSON Report Compiler Unit Tests ==="

# Create a temporary mock text report file
MOCK_TXT=$(mktemp)

cat << 'EOF' > "$MOCK_TXT"
=== ByteTaper Benchmark Execution ===
Scenario: unit_test_scenario
Time: Wed May  6 14:17:16 UTC 2026
Target Host: http://envoy-unit-test:10000/api/v1/cached/fast/bench

=== System Information ===
OS: Linux f6294ea67bdc 6.11.3-200.fc40.aarch64 aarch64 Linux
CPU cores: 4
Memory Total: 8192 MB

=== Parsed Scenario Metrics ===
Leg 1 Latency JSON: {"latency_ms":{"p50":10.5,"p95":20.0,"p99":50.0}}
Leg 1 Throughput JSON: {"throughput":{"requests_per_second":120.5,"total_requests":1200,"successful_requests":1200,"failed_requests":0}}
Leg 1 Container Stats JSON: {"envoy":{"cpu_percent":5.5,"peak_memory_mb":12.5},"bytetaper-extproc":{"cpu_percent":8.2,"peak_memory_mb":18.4},"mock-api":{"cpu_percent":2.1,"peak_memory_mb":9.0}}
Leg 1 Payload Savings JSON: {"original_bytes_avg":2048,"optimized_bytes_avg":1024,"bytes_saved_avg":1024,"reduction_ratio":"50.00%"}

Leg 2 Latency JSON: {"latency_ms":{"p50":15.2,"p95":25.4,"p99":60.8}}
Leg 2 Throughput JSON: {"throughput":{"requests_per_second":110.0,"total_requests":1100,"successful_requests":1100,"failed_requests":0}}
Leg 2 Container Stats JSON: {"envoy":{"cpu_percent":6.0,"peak_memory_mb":13.0},"bytetaper-extproc":{"cpu_percent":9.0,"peak_memory_mb":19.0},"mock-api":{"cpu_percent":2.5,"peak_memory_mb":9.5}}
Leg 2 Payload Savings JSON: {"original_bytes_avg":4096,"optimized_bytes_avg":4096,"bytes_saved_avg":0,"reduction_ratio":"0.00%"}
EOF

# Run generator
./benchmarks/report/generate_json_report.sh "$MOCK_TXT"

MOCK_JSON="${MOCK_TXT%.txt}.json"

if [ ! -f "$MOCK_JSON" ]; then
    echo "FAIL: Output JSON report file not created" >&2
    exit 1
fi

# --------------------------------------------------
# Test Assertions
# --------------------------------------------------
assert_equals "1.0.0" "$(jq -r '.benchmark_version' "$MOCK_JSON")" "Verify benchmark_version"
assert_equals "unit_test_scenario" "$(jq -r '.scenario' "$MOCK_JSON")" "Verify parsed scenario name"
assert_equals "Wed May  6 14:17:16 UTC 2026" "$(jq -r '.timestamp' "$MOCK_JSON")" "Verify timestamp"

# Features section
assert_equals "Linux f6294ea67bdc 6.11.3-200.fc40.aarch64 aarch64 Linux" "$(jq -r '.features.os_info' "$MOCK_JSON")" "Verify features: os_info"
assert_equals "4" "$(jq -r '.features.cpu_cores' "$MOCK_JSON")" "Verify features: cpu_cores is an integer/numeric"
assert_equals "8192 MB" "$(jq -r '.features.memory_total' "$MOCK_JSON")" "Verify features: memory_total"
assert_equals "http://envoy-unit-test:10000/api/v1/cached/fast/bench" "$(jq -r '.features.target_host' "$MOCK_JSON")" "Verify features: target_host"

# Leg 1 Metrics
assert_equals "10.5" "$(jq -r '.latency_ms."Leg 1".latency_ms.p50' "$MOCK_JSON")" "Leg 1: p50 latency matches"
assert_equals "120.5" "$(jq -r '.throughput."Leg 1".throughput.requests_per_second' "$MOCK_JSON")" "Leg 1: throughput rps matches"
assert_equals "8.2" "$(jq -r '.resources."Leg 1"."bytetaper-extproc".cpu_percent' "$MOCK_JSON")" "Leg 1: extproc CPU percent matches"
assert_equals "50.00%" "$(jq -r '.payload."Leg 1".reduction_ratio' "$MOCK_JSON")" "Leg 1: payload reduction ratio matches"

# Leg 2 Metrics
assert_equals "15.2" "$(jq -r '.latency_ms."Leg 2".latency_ms.p50' "$MOCK_JSON")" "Leg 2: p50 latency matches"
assert_equals "110.0" "$(jq -r '.throughput."Leg 2".throughput.requests_per_second' "$MOCK_JSON")" "Leg 2: throughput rps matches"
assert_equals "9.0" "$(jq -r '.resources."Leg 2"."bytetaper-extproc".cpu_percent' "$MOCK_JSON")" "Leg 2: extproc CPU percent matches"
assert_equals "0.00%" "$(jq -r '.payload."Leg 2".reduction_ratio' "$MOCK_JSON")" "Leg 2: payload reduction ratio matches"

# Cleanup
rm -f "$MOCK_TXT" "$MOCK_JSON"

echo "=== All JSON Report Compiler Unit Tests Passed ==="
exit 0
