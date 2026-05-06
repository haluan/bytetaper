#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Helper to run test assertion
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

echo "=== Running Throughput Parser Unit Tests ==="

# --------------------------------------------------
# Test Case 1: Normal wrk output with no errors
# --------------------------------------------------
TMP_WRK_NORMAL=$(mktemp)
cat <<EOF > "$TMP_WRK_NORMAL"
Running 10s test @ http://envoy:10000/api/v1/large
  2 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   116.88ms   29.63ms 141.56ms   90.88%
    Req/Sec    43.85     16.12    60.00     53.23%
  855 requests in 10.03s, 1.39MB read
Requests/sec:     85.23
Transfer/sec:    142.07KB
EOF

JSON_NORMAL=$(./benchmarks/lib/throughput_parser.sh "$TMP_WRK_NORMAL")
assert_equals "85.23" "$(echo "$JSON_NORMAL" | jq -r '.throughput.requests_per_second')" "Normal output: requests_per_second parsed"
assert_equals "855" "$(echo "$JSON_NORMAL" | jq -r '.throughput.total_requests')" "Normal output: total_requests parsed"
assert_equals "855" "$(echo "$JSON_NORMAL" | jq -r '.throughput.successful_requests')" "Normal output: successful_requests parsed"
assert_equals "0" "$(echo "$JSON_NORMAL" | jq -r '.throughput.failed_requests')" "Normal output: failed_requests parsed"
assert_equals "855" "$(echo "$JSON_NORMAL" | jq -r '.total_requests')" "Normal output: top-level total_requests parsed"

# --------------------------------------------------
# Test Case 2: wrk output with Non-2xx/3xx responses
# --------------------------------------------------
TMP_WRK_NON2XX=$(mktemp)
cat <<EOF > "$TMP_WRK_NON2XX"
Running 10s test @ http://envoy:10000/api/v1/large
  2 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   116.88ms   29.63ms 141.56ms   90.88%
    Req/Sec    43.85     16.12    60.00     53.23%
  855 requests in 10.03s, 1.39MB read
  Non-2xx or 3xx responses: 15
Requests/sec:     85.23
Transfer/sec:    142.07KB
EOF

JSON_NON2XX=$(./benchmarks/lib/throughput_parser.sh "$TMP_WRK_NON2XX")
assert_equals "85.23" "$(echo "$JSON_NON2XX" | jq -r '.throughput.requests_per_second')" "Non-2xx output: rps parsed"
assert_equals "855" "$(echo "$JSON_NON2XX" | jq -r '.throughput.total_requests')" "Non-2xx output: total parsed"
assert_equals "840" "$(echo "$JSON_NON2XX" | jq -r '.throughput.successful_requests')" "Non-2xx output: success count parsed"
assert_equals "15" "$(echo "$JSON_NON2XX" | jq -r '.throughput.failed_requests')" "Non-2xx output: failed count parsed"

# --------------------------------------------------
# Test Case 3: wrk output with Socket errors
# --------------------------------------------------
TMP_WRK_SOCKET=$(mktemp)
cat <<EOF > "$TMP_WRK_SOCKET"
Running 10s test @ http://envoy:10000/api/v1/large
  2 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   116.88ms   29.63ms 141.56ms   90.88%
    Req/Sec    43.85     16.12    60.00     53.23%
  855 requests in 10.03s, 1.39MB read
  Socket errors: connect 2, read 3, write 0, timeout 4
Requests/sec:     85.23
Transfer/sec:    142.07KB
EOF

JSON_SOCKET=$(./benchmarks/lib/throughput_parser.sh "$TMP_WRK_SOCKET")
assert_equals "855" "$(echo "$JSON_SOCKET" | jq -r '.throughput.total_requests')" "Socket errors output: total parsed"
assert_equals "9" "$(echo "$JSON_SOCKET" | jq -r '.throughput.failed_requests')" "Socket errors output: failed count parsed (sum of errors)"
assert_equals "846" "$(echo "$JSON_SOCKET" | jq -r '.throughput.successful_requests')" "Socket errors output: success count parsed"

# --------------------------------------------------
# Test Case 4: wrk output with both Non-2xx and Socket errors
# --------------------------------------------------
TMP_WRK_BOTH=$(mktemp)
cat <<EOF > "$TMP_WRK_BOTH"
Running 10s test @ http://envoy:10000/api/v1/large
  2 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   116.88ms   29.63ms 141.56ms   90.88%
    Req/Sec    43.85     16.12    60.00     53.23%
  1000 requests in 10.03s, 1.39MB read
  Non-2xx or 3xx responses: 20
  Socket errors: connect 5, read 0, write 0, timeout 10
Requests/sec:     99.71
Transfer/sec:    142.07KB
EOF

JSON_BOTH=$(./benchmarks/lib/throughput_parser.sh "$TMP_WRK_BOTH")
assert_equals "99.71" "$(echo "$JSON_BOTH" | jq -r '.throughput.requests_per_second')" "Both errors: rps parsed"
assert_equals "1000" "$(echo "$JSON_BOTH" | jq -r '.throughput.total_requests')" "Both errors: total parsed"
assert_equals "35" "$(echo "$JSON_BOTH" | jq -r '.throughput.failed_requests')" "Both errors: failed count parsed (20 non-2xx + 15 socket)"
assert_equals "965" "$(echo "$JSON_BOTH" | jq -r '.throughput.successful_requests')" "Both errors: success count parsed"

# --------------------------------------------------
# Test Case 5: Missing or incomplete file (Negative cases)
# --------------------------------------------------
TMP_WRK_BAD=$(mktemp)
echo "Some garbage lines" > "$TMP_WRK_BAD"

if ./benchmarks/lib/throughput_parser.sh "$TMP_WRK_BAD" 2>/dev/null; then
    echo "FAIL: Expected parser to fail on incomplete/invalid file" >&2
    exit 1
else
    echo "PASS: Parser failed on incomplete/invalid file as expected"
fi

if ./benchmarks/lib/throughput_parser.sh "nonexistent_file_path" 2>/dev/null; then
    echo "FAIL: Expected parser to fail on nonexistent file" >&2
    exit 1
else
    echo "PASS: Parser failed on nonexistent file as expected"
fi

# Cleanup
rm -f "$TMP_WRK_NORMAL" "$TMP_WRK_NON2XX" "$TMP_WRK_SOCKET" "$TMP_WRK_BOTH" "$TMP_WRK_BAD"

echo "=== All Throughput Parser Unit Tests Passed ==="
exit 0
