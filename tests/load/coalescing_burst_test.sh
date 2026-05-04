#!/bin/bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -eu

# Configuration
ENVOY_URL="${BYTETAPER_TEST_BASE_URL:-http://envoy:10000}"
MOCK_RESET_URL="${BYTETAPER_MOCK_RESET_URL:-http://mock-api:8080/reset-count}"
MOCK_COUNT_URL="${BYTETAPER_MOCK_COUNTER_URL:-http://mock-api:8080/call-count}"
METRICS_URL="http://bytetaper-extproc:18081/metrics"

echo "=== Coalescing Burst Load Test ==="

# Helper to check metrics
check_metric() {
  local name=$1
  local metrics
  metrics=$(curl -s "$METRICS_URL")
  if echo "$metrics" | grep -q "^${name}"; then
    local val=$(echo "$metrics" | grep "^${name}" | awk '{print $2}')
    echo "Metric $name = $val"
    return 0
  else
    echo "Metric $name not found"
    return 1
  fi
}

# 1. Reset state
echo "Resetting mock API counter..."
curl -s "$MOCK_RESET_URL" > /dev/null

# 2. Burst Phase 1: Successful Coalescing (Fast)
N1=5
URL1="${ENVOY_URL}/api/v1/cached/fast/burst-1"
echo "Phase 1: Sending $N1 concurrent requests to $URL1"

python3 -c "
import threading, urllib.request, time
def req():
    try: urllib.request.urlopen('$URL1').read()
    except Exception as e: print(e)

threads = [threading.Thread(target=req) for _ in range($N1)]
for t in threads: t.start()
for t in threads: t.join()
"

COUNT1=$(curl -s "$MOCK_COUNT_URL")
echo "Upstream calls after Phase 1: $COUNT1"

if [ "$COUNT1" -ge "$N1" ]; then
  echo "FAIL: No coalescing observed in Phase 1 (Count: $COUNT1, N: $N1)"
  echo "Current Metrics:"
  curl -s "$METRICS_URL"
  exit 1
fi

if [ "$COUNT1" -eq 0 ]; then
  echo "FAIL: Upstream was never called in Phase 1"
  exit 1
fi

# 3. Burst Phase 2: Fallback Coalescing (Slow)
# /slow/ path triggers 100ms delay in mock API, exceeding 50ms wait window.
N2=5
URL2="${ENVOY_URL}/api/v1/cached/slow/burst-2"
echo "Phase 2: Sending $N2 concurrent requests to $URL2"

# Reset counter for Phase 2 clarity
curl -s "$MOCK_RESET_URL" > /dev/null

for i in $(seq 1 $N2); do
  curl -s "$URL2" > /dev/null &
done

echo "Waiting for background requests to complete..."
wait

COUNT2=$(curl -s "$MOCK_COUNT_URL")
echo "Upstream calls in Phase 2: $COUNT2"

# We expect at least one fallback here because the 100ms delay > 50ms window.
# Actually, if they all fallback, count will be 5.
# If Req 1 is leader, Req 2-5 arrive later and wait 50ms. Req 1 still hasn't finished (needs 100ms).
# So Req 2-5 timeout and fallback. Total = 1 (Leader) + 4 (Fallbacks) = 5.

# 4. Metrics Verification
echo "Verifying Prometheus metrics..."
check_metric "bytetaper_coalescing_leader_total"
check_metric "bytetaper_coalescing_follower_total"
check_metric "bytetaper_coalescing_fallback_total"

# Final check
TOTAL_CALLS=$(curl -s "$MOCK_COUNT_URL")
echo "=== Test Results ==="
echo "Phase 1 reduction: $COUNT1 / $N1"
echo "Phase 2 results: $COUNT2 / $N2"

if [ "$COUNT1" -lt "$N1" ]; then
  echo "PASS: Load test successful"
  exit 0
else
  echo "FAIL: Reduction goals not met"
  exit 1
fi
