#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Configuration
SCENARIO="envoy_only"
TARGET_URL="${ENVOY_BASELINE_URL:-http://envoy-baseline:10000}/api/v1/cached/fast/bench"
REPORT_DIR="reports/benchmarks"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="${REPORT_DIR}/benchmark_results_${TIMESTAMP}_${SCENARIO}.txt"

mkdir -p "$REPORT_DIR"

# Collect system info
{
    echo "=== ByteTaper Benchmark Execution ==="
    echo "Scenario: $SCENARIO"
    echo "Time: $(date)"
    echo "Target: $TARGET_URL"
    echo ""
    echo "=== System Information ==="
    echo "OS: $(uname -snrmo)"
    echo "CPU cores: $(nproc)"
    echo "Memory Total: $(grep MemTotal /proc/meminfo | awk '{print $2/1024 " MB"}')"
    echo ""
} > "$REPORT_FILE"

# Print system info to console
cat "$REPORT_FILE"

# Check target availability
echo "Checking target availability..."
if ! curl -s --fail "$TARGET_URL" > /dev/null; then
    echo "ERROR: Benchmark target $TARGET_URL is unavailable."
    echo "Status: UNAVAILABLE" >> "$REPORT_FILE"
    exit 1
fi

echo "Target is UP. Starting wrk..."

# Run wrk and capture raw output to a temporary file
WRK_OUT=$(mktemp)
wrk -t2 -c10 -d10s --latency "$TARGET_URL" | tee "$WRK_OUT"

# Append raw wrk output to report file
cat "$WRK_OUT" >> "$REPORT_FILE"

# Parse metrics
echo "" >> "$REPORT_FILE"
echo "=== Parsed Scenario Metrics ===" >> "$REPORT_FILE"

# Total requests
total_reqs=$(grep -E '^[[:space:]]*[0-9]+ requests in' "$WRK_OUT" | awk '{print $1}')
if [ -z "$total_reqs" ]; then
    total_reqs=0
fi

# Non-2xx/3xx if present
non_2xx_3xx=$(grep -E "Non-2xx or 3xx responses:" "$WRK_OUT" | awk '{print $5}')
if [ -z "$non_2xx_3xx" ]; then
    non_2xx_3xx=0
fi

# Success count
success_count=$((total_reqs - non_2xx_3xx))

# Latencies
avg_latency=$(grep -E "^[[:space:]]*Latency" "$WRK_OUT" | awk '{print $2}')
p50_latency=$(grep -E "^[[:space:]]*50%" "$WRK_OUT" | awk '{print $2}')
p75_latency=$(grep -E "^[[:space:]]*75%" "$WRK_OUT" | awk '{print $2}')
p90_latency=$(grep -E "^[[:space:]]*90%" "$WRK_OUT" | awk '{print $2}')
p99_latency=$(grep -E "^[[:space:]]*99%" "$WRK_OUT" | awk '{print $2}')

# Bytes read & transfer rate
bytes_read=$(grep -E '^[[:space:]]*[0-9]+ requests in' "$WRK_OUT" | awk -F', ' '{print $2}')
transfer_rate=$(grep -E "^Transfer/sec:" "$WRK_OUT" | awk '{print $2}')

{
    echo "Total Requests: $total_reqs"
    echo "Success Count: $success_count"
    echo "Latency Average: $avg_latency"
    echo "Latency p50: $p50_latency"
    echo "Latency p75: $p75_latency"
    echo "Latency p90: $p90_latency"
    echo "Latency p99: $p99_latency"
    echo "Bytes Read: $bytes_read"
    echo "Transfer Rate: $transfer_rate"
} >> "$REPORT_FILE"

# Cleanup
rm -f "$WRK_OUT"

echo ""
echo "Benchmark complete."
echo "Results saved to: $REPORT_FILE"
cat "$REPORT_FILE" | tail -n 12
