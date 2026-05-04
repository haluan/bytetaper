#!/bin/bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -eu

ENVOY_URL="${BYTETAPER_TEST_BASE_URL:-http://envoy:10000}"
MOCK_RESET_URL="${BYTETAPER_MOCK_RESET_URL:-http://mock-api:8080/reset-count}"
MOCK_COUNT_URL="${BYTETAPER_MOCK_COUNTER_URL:-http://mock-api:8080/call-count}"

echo "=== Coalescing E2E Test ==="

# Helper to run python script for concurrent requests
run_concurrent() {
    local url=$1
    local method=$2
    local count=$3
    python3 -c "
import threading, urllib.request, sys
errors = []
def req():
    req = urllib.request.Request('$url', method='$method')
    try:
        res = urllib.request.urlopen(req)
        if res.getcode() != 200:
            errors.append(f'Status {res.getcode()}')
    except Exception as e:
        errors.append(str(e))

threads = [threading.Thread(target=req) for _ in range($count)]
for t in threads: t.start()
for t in threads: t.join()
if errors:
    print(f'Errors encountered: {errors}')
    sys.exit(1)
"
}

# 1. Reset state
echo "Resetting mock API counter..."
curl -s "$MOCK_RESET_URL" > /dev/null

# Scenario 1: Concurrent GET requests (Fast Upstream)
# Expectation: Upstream calls reduced (coalesced)
echo "Scenario 1: Concurrent GET requests (Fast Upstream)"
N1=5
run_concurrent "${ENVOY_URL}/api/v1/cached/fast/e2e-1" "GET" $N1
COUNT1=$(curl -s "$MOCK_COUNT_URL")
echo "Upstream calls: $COUNT1 / $N1"

if [ "$COUNT1" -lt "$N1" ]; then
    echo "PASS: Upstream calls reduced"
else
    echo "FAIL: Upstream calls not reduced"
    exit 1
fi

# Scenario 2: Concurrent GET requests (Slow Upstream - Fallback)
# 100ms upstream delay > 50ms wait window.
# Expectation: Client still receives 200 OK (safety fallback)
echo "Scenario 2: Concurrent GET requests (Slow Upstream - Fallback)"
curl -s "$MOCK_RESET_URL" > /dev/null
N2=5
run_concurrent "${ENVOY_URL}/api/v1/cached/slow/e2e-2" "GET" $N2
COUNT2=$(curl -s "$MOCK_COUNT_URL")
echo "PASS: Fallback successful (Upstream calls: $COUNT2 / $N2)"

# Scenario 3: Concurrent POST requests (Non-GET)
# Expectation: No coalescing for non-GET requests
echo "Scenario 3: Concurrent POST requests (Non-GET)"
curl -s "$MOCK_RESET_URL" > /dev/null
N3=3
run_concurrent "${ENVOY_URL}/api/v1/cached/fast/e2e-3" "POST" $N3
COUNT3=$(curl -s "$MOCK_COUNT_URL")
echo "Upstream calls: $COUNT3 / $N3"

if [ "$COUNT3" -eq "$N3" ]; then
    echo "PASS: Non-GET requests not coalesced"
else
    echo "FAIL: Non-GET requests unexpectedly coalesced"
    exit 1
fi

echo "ALL COALESCING E2E SCENARIOS PASSED"
