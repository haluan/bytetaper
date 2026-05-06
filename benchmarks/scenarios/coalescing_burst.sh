#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Configuration
SCENARIO="coalescing_burst"
ENVOY_HOST="${ENVOY_COALESCING_URL:-http://envoy-bytetaper-coalescing:10000}"
METRICS_HOST="${METRICS_COALESCING_URL:-http://bytetaper-extproc-coalescing:18081}"
MOCK_HOST="${MOCK_URL:-http://mock-api:8080}"
REPORT_DIR="reports/benchmarks"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="${REPORT_DIR}/benchmark_results_${TIMESTAMP}_${SCENARIO}.txt"

mkdir -p "$REPORT_DIR"

# Collect system info
{
    echo "=== ByteTaper Benchmark Execution ==="
    echo "Scenario: $SCENARIO"
    echo "Time: $(date)"
    echo "Target Envoy: $ENVOY_HOST"
    echo "Metrics Host: $METRICS_HOST"
    echo ""
    echo "=== System Information ==="
    echo "OS: $(uname -snrmo)"
    echo "CPU cores: $(nproc)"
    echo "Memory Total: $(grep MemTotal /proc/meminfo | awk '{print $2/1024 " MB"}' || echo 'N/A')"
    echo ""
} > "$REPORT_FILE"

# Print system info to console
cat "$REPORT_FILE"

# Check targets are UP
echo "Checking target availability..."
if ! curl -s --fail "${ENVOY_HOST}/products/fast/123" > /dev/null; then
    echo "ERROR: Target Envoy at ${ENVOY_HOST} is unavailable."
    echo "Status: UNAVAILABLE" >> "$REPORT_FILE"
    exit 1
fi
if ! curl -s --fail "${METRICS_HOST}/metrics" > /dev/null; then
    echo "ERROR: Metrics server at ${METRICS_HOST} is unavailable."
    echo "Status: UNAVAILABLE" >> "$REPORT_FILE"
    exit 1
fi
if ! curl -s --fail "${MOCK_HOST}/call-count" > /dev/null; then
    echo "ERROR: Mock API at ${MOCK_HOST} is unavailable."
    echo "Status: UNAVAILABLE" >> "$REPORT_FILE"
    exit 1
fi

echo "All targets are UP."

# Metric extraction function
get_metric() {
    local name=$1
    local val
    val=$(curl -s "${METRICS_HOST}/metrics" | grep "^${name}" | awk '{print $2}' || echo "0")
    if [ -z "$val" ]; then val=0; fi
    echo "$val"
}

# --------------------------------------------------
# Leg A: Successful Request Coalescing (fast path, 20ms sleep)
# --------------------------------------------------
echo "--------------------------------------------------" | tee -a "$REPORT_FILE"
echo "Leg A: Successful Request Coalescing Burst (fast path)" | tee -a "$REPORT_FILE"
echo "--------------------------------------------------" | tee -a "$REPORT_FILE"

# Record baseline
base_leader=$(get_metric "bytetaper_coalescing_leader_total")
base_follower=$(get_metric "bytetaper_coalescing_follower_total")
base_cache_hit=$(get_metric "bytetaper_coalescing_follower_cache_hit_total")
base_fallback=$(get_metric "bytetaper_coalescing_fallback_total")
base_bypass=$(get_metric "bytetaper_coalescing_bypass_total")

# Reset mock api counter
curl -s "${MOCK_HOST}/reset-count" > /dev/null

N=50
echo "Sending $N concurrent GET requests to ${ENVOY_HOST}/products/fast/123 ..."

# Concurrent execution loop
for i in $(seq 1 "$N"); do
    curl -s -o /dev/null "${ENVOY_HOST}/products/fast/123" &
done
wait

echo "Concurrent burst complete."
echo "Gathering metrics..."

# Query new values
new_leader=$(get_metric "bytetaper_coalescing_leader_total")
new_follower=$(get_metric "bytetaper_coalescing_follower_total")
new_cache_hit=$(get_metric "bytetaper_coalescing_follower_cache_hit_total")
new_fallback=$(get_metric "bytetaper_coalescing_fallback_total")
new_bypass=$(get_metric "bytetaper_coalescing_bypass_total")
mock_calls=$(curl -s "${MOCK_HOST}/call-count")

# Compute delta
delta_leader=$((new_leader - base_leader))
delta_follower=$((new_follower - base_follower))
delta_cache_hit=$((new_cache_hit - base_cache_hit))
delta_fallback=$((new_fallback - base_fallback))
delta_bypass=$((new_bypass - base_bypass))

echo "Running wrk latency check on fast path..."
WRK_COAL_A=$(mktemp)
wrk -t2 -c5 -d3s -s benchmarks/lib/latency_reporter.lua --latency "${ENVOY_HOST}/products/fast/123" | tee "$WRK_COAL_A"
JSON_COAL_A=$(./benchmarks/lib/latency_parser.sh "$WRK_COAL_A")
rm -f "$WRK_COAL_A"

{
    echo "Client Requests Sent: $N"
    echo "Upstream Mock Calls: $mock_calls"
    echo "Delta Leaders: $delta_leader"
    echo "Delta Followers: $delta_follower"
    echo "Delta Follower Cache Hits: $delta_cache_hit"
    echo "Delta Fallbacks: $delta_fallback"
    echo "Delta Bypasses: $delta_bypass"
    echo "Leg A Latency JSON: $JSON_COAL_A"
    echo ""
} | tee -a "$REPORT_FILE"

# Assertions
if [ "$delta_leader" -eq 0 ]; then
    echo "ERROR: Expected at least 1 leader request, got 0"
    exit 1
fi
if [ "$delta_follower" -eq 0 ]; then
    echo "ERROR: Expected some followers for overlapping concurrent requests, got 0"
    exit 1
fi

# Sleep to cool down socket pools
echo "Cooling down socket pools..."
sleep 3

# --------------------------------------------------
# Leg B: Request Coalescing Timeout Fallback (slow path, 100ms sleep)
# --------------------------------------------------
echo "--------------------------------------------------" | tee -a "$REPORT_FILE"
echo "Leg B: Request Coalescing Timeout Fallback (slow path)" | tee -a "$REPORT_FILE"
echo "--------------------------------------------------" | tee -a "$REPORT_FILE"

# Record baseline
base_leader=$(get_metric "bytetaper_coalescing_leader_total")
base_follower=$(get_metric "bytetaper_coalescing_follower_total")
base_cache_hit=$(get_metric "bytetaper_coalescing_follower_cache_hit_total")
base_fallback=$(get_metric "bytetaper_coalescing_fallback_total")
base_bypass=$(get_metric "bytetaper_coalescing_bypass_total")

# Reset mock api counter
curl -s "${MOCK_HOST}/reset-count" > /dev/null

echo "Sending $N concurrent GET requests to ${ENVOY_HOST}/products/slow/123 ..."

# Concurrent execution loop
for i in $(seq 1 "$N"); do
    curl -s -o /dev/null "${ENVOY_HOST}/products/slow/123" &
done
wait

echo "Concurrent burst complete."
echo "Gathering metrics..."

# Query new values
new_leader=$(get_metric "bytetaper_coalescing_leader_total")
new_follower=$(get_metric "bytetaper_coalescing_follower_total")
new_cache_hit=$(get_metric "bytetaper_coalescing_follower_cache_hit_total")
new_fallback=$(get_metric "bytetaper_coalescing_fallback_total")
new_bypass=$(get_metric "bytetaper_coalescing_bypass_total")
mock_calls=$(curl -s "${MOCK_HOST}/call-count")

# Compute delta
delta_leader=$((new_leader - base_leader))
delta_follower=$((new_follower - base_follower))
delta_cache_hit=$((new_cache_hit - base_cache_hit))
delta_fallback=$((new_fallback - base_fallback))
delta_bypass=$((new_bypass - base_bypass))

echo "Running wrk latency check on slow path..."
WRK_COAL_B=$(mktemp)
wrk -t2 -c5 -d3s -s benchmarks/lib/latency_reporter.lua --latency "${ENVOY_HOST}/products/slow/123" | tee "$WRK_COAL_B"
JSON_COAL_B=$(./benchmarks/lib/latency_parser.sh "$WRK_COAL_B")
JSON_COAL_TP_B=$(./benchmarks/lib/throughput_parser.sh "$WRK_COAL_B")
rm -f "$WRK_COAL_B"

# Extract container stats for Leg B
echo "Extracting container stats for Leg B..."
JSON_COAL_STATS_B=$(./benchmarks/lib/container_stats.sh all)

# Extract response payload savings for Leg B
echo "Extracting payload savings for Leg B..."
coal_b_size=$(curl -s -o /dev/null -w "%{size_download}" "${ENVOY_HOST}/products/slow/123" || echo "0")
JSON_COAL_SAVINGS_B=$(./benchmarks/lib/payload_savings_parser.sh "$coal_b_size" "$coal_b_size")

{
    echo "Client Requests Sent: $N"
    echo "Upstream Mock Calls: $mock_calls"
    echo "Delta Leaders: $delta_leader"
    echo "Delta Followers: $delta_follower"
    echo "Delta Follower Cache Hits: $delta_cache_hit"
    echo "Delta Fallbacks: $delta_fallback"
    echo "Delta Bypasses: $delta_bypass"
    echo "Leg B Latency JSON: $JSON_COAL_B"
    echo "Leg B Throughput JSON: $JSON_COAL_TP_B"
    echo "Leg B Container Stats JSON: $JSON_COAL_STATS_B"
    echo "Leg B Payload Savings JSON: $JSON_COAL_SAVINGS_B"
    echo ""
} | tee -a "$REPORT_FILE"

# Assertions
if [ "$delta_fallback" -eq 0 ]; then
    echo "ERROR: Expected at least some fallbacks due to 100ms mock delay exceeding 25ms wait window, got 0"
    exit 1
fi

# Validate report integrity
./benchmarks/report/validate.sh "$REPORT_FILE"

# Compile JSON report
./benchmarks/report/generate_json_report.sh "$REPORT_FILE"

echo "Benchmark complete."
echo "Results saved to: $REPORT_FILE"
