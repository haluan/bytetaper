#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Path to parser script
PARSER_SCRIPT="./benchmarks/lib/latency_parser.sh"

echo "=== Running Latency Parser Unit Tests ==="

# Helper to source parser or invoke as standalone
source "$PARSER_SCRIPT"

# 1. Test unit conversion helper (to_ms)
echo "Testing to_ms unit conversion helper..."
res_ms=$(to_ms "2.5ms")
if [ "$res_ms" != "2.5" ]; then
    echo "ERROR: to_ms '2.5ms' returned '$res_ms' (expected '2.5')" >&2
    exit 1
fi

res_us=$(to_ms "1500us")
if [ "$res_us" != "1.5" ]; then
    echo "ERROR: to_ms '1500us' returned '$res_us' (expected '1.5')" >&2
    exit 1
fi

res_s=$(to_ms "1.5s")
if [ "$res_s" != "1500" ]; then
    echo "ERROR: to_ms '1.5s' returned '$res_s' (expected '1500')" >&2
    exit 1
fi

res_num=$(to_ms "10.2")
if [ "$res_num" != "10.2" ]; then
    echo "ERROR: to_ms '10.2' returned '$res_num' (expected '10.2')" >&2
    exit 1
fi
echo "Unit conversion tests passed."

# 2. Test positive case with custom Lua printed percentiles
echo "Testing positive case with custom Lua percentiles..."
temp_ok=$(mktemp)
cat <<EOF > "$temp_ok"
Running 10s test @ http://localhost:10000
  2 threads and 10 connections
  Latency   1.20ms    0.50ms   5.00ms   80.0%
LATENCY_PERCENTILES: p50=1.125 ms, p95=2.450 ms, p99=4.890 ms
EOF

json_out=$(parse_latency "$temp_ok")
echo "Parser output:"
echo "$json_out"

# Validate using jq or direct matching
p50_val=$(echo "$json_out" | jq -r '.latency_ms.p50')
p95_val=$(echo "$json_out" | jq -r '.latency_ms.p95')
p99_val=$(echo "$json_out" | jq -r '.latency_ms.p99')

if [ "$p50_val" != "1.125" ] || [ "$p95_val" != "2.450" ] || [ "$p99_val" != "4.890" ]; then
    echo "ERROR: Parsed values do not match input (p50='$p50_val', p95='$p95_val', p99='$p99_val')" >&2
    exit 1
fi
echo "Positive case with custom percentiles passed."

# 3. Test negative case (missing p95 should fail)
echo "Testing negative case (missing p95 percentile)..."
temp_fail=$(mktemp)
cat <<EOF > "$temp_fail"
Running 10s test @ http://localhost:10000
  2 threads and 10 connections
  Latency Distribution
     50%    1.50ms
     75%    2.10ms
     90%    3.40ms
     99%    5.02ms
EOF

if parse_latency "$temp_fail" 2>/dev/null; then
    echo "ERROR: parse_latency succeeded despite missing p95 data!" >&2
    exit 1
fi
echo "Negative case (missing p95) handled successfully (failed as expected)."

# 4. Test missing file case (should fail)
echo "Testing missing file handling..."
if parse_latency "non_existent_file_xyz.txt" 2>/dev/null; then
    echo "ERROR: parse_latency succeeded on non-existent file!" >&2
    exit 1
fi
echo "Missing file handled successfully."

# Cleanup temp files
rm -f "$temp_ok" "$temp_fail"

echo "=== All Latency Parser Unit Tests PASSED ==="
