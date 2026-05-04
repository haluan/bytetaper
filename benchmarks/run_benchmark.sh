#!/bin/bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -eu

TARGET_URL="${ENVOY_URL:-http://envoy:10000}/api/v1/cached/fast/bench"
REPORT_DIR="reports/benchmarks"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="${REPORT_DIR}/benchmark_results_${TIMESTAMP}.txt"

mkdir -p "$REPORT_DIR"

# Collect system info
{
    echo "=== ByteTaper Benchmark Execution ==="
    echo "Time: $(date)"
    echo "Target: $TARGET_URL"
    echo ""
    echo "=== System Information ==="
    echo "OS: $(uname -snrmo)"
    echo "CPU cores: $(nproc)"
    echo "Memory Total: $(grep MemTotal /proc/meminfo | awk '{print $2/1024 " MB"}')"
    echo ""
} > "$REPORT_FILE"

# Print to console as well
cat "$REPORT_FILE"

# Check target availability
echo "Checking target availability..."
if ! curl -s --fail "$TARGET_URL" > /dev/null; then
    echo "ERROR: Benchmark target $TARGET_URL is unavailable."
    echo "Status: UNAVAILABLE" >> "$REPORT_FILE"
    exit 1
fi

echo "Target is UP. Starting wrk..."

# Run wrk and append to report
# -t2: 2 threads
# -c10: 10 connections
# -d10s: 10 seconds duration
# --latency: print detailed latency statistics
wrk -t2 -c10 -d10s --latency "$TARGET_URL" | tee -a "$REPORT_FILE"

echo ""
echo "Benchmark complete."
echo "Results saved to: $REPORT_FILE"
