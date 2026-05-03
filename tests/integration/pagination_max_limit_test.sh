#!/usr/bin/env sh
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -eu

ENVOY_HOST="${ENVOY_HOST:-envoy}"
ENVOY_PORT="${ENVOY_PORT:-10000}"
BASE="http://${ENVOY_HOST}:${ENVOY_PORT}"

# --- Scenario A: excessive limit → ByteTaper caps to 500 ---

echo "Testing Scenario A: excessive limit (5000) -> should be capped to 500"
response=$(curl -s -D - "${BASE}/orders?limit=5000")
http_code=$(printf '%s' "$response" | grep "^HTTP" | tail -1 | awk '{print $2}')
headers=$(printf '%s' "$response" | sed '/^\r$/q')
body=$(printf '%s' "$response" | sed '1,/^\r$/d')

[ "$http_code" = "200" ] || { echo "FAIL: expected 200, got $http_code"; exit 1; }

printf '%s\n' "$headers" | grep -qi "x-bytetaper-pagination-applied: true" \
  || { echo "FAIL: x-bytetaper-pagination-applied header missing or not true"; exit 1; }

printf '%s\n' "$headers" | grep -qi "x-bytetaper-pagination-reason: limit_exceeds_max" \
  || { echo "FAIL: x-bytetaper-pagination-reason not limit_exceeds_max"; exit 1; }

printf '%s\n' "$body" | grep -q '"received_limit": "500"' \
  || printf '%s\n' "$body" | grep -q '"received_limit":"500"' \
  || { echo "FAIL: upstream did not receive capped limit=500; body: $body"; exit 1; }

# --- Scenario B: valid limit → upstream receives it unchanged ---

echo "Testing Scenario B: valid limit (100) -> should be unchanged"
response2=$(curl -s -D - "${BASE}/orders?limit=100")
headers2=$(printf '%s' "$response2" | sed '/^\r$/q')
body2=$(printf '%s' "$response2" | sed '1,/^\r$/d')

printf '%s\n' "$headers2" | grep -qi "x-bytetaper-pagination-applied: true" \
  && { echo "FAIL: pagination-applied must be absent for valid limit"; exit 1; } || true

printf '%s\n' "$body2" | grep -q '"received_limit": "100"' \
  || printf '%s\n' "$body2" | grep -q '"received_limit":"100"' \
  || { echo "FAIL: valid limit was mutated; body: $body2"; exit 1; }

echo "PASS: max limit enforcement verified"
