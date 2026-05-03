#!/usr/bin/env sh
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -eu

ENVOY_HOST="${ENVOY_HOST:-envoy}"
ENVOY_PORT="${ENVOY_PORT:-10000}"
BASE="http://${ENVOY_HOST}:${ENVOY_PORT}"

# --- Scenario A: no limit → ByteTaper injects ?limit=50 ---

echo "Scenario A: Testing default limit injection..."
response=$(curl -s -D - "${BASE}/orders")
http_code=$(printf '%s' "$response" | grep "^HTTP" | tail -1 | awk '{print $2}')
headers=$(printf '%s' "$response" | sed '/^\r$/q')
body=$(printf '%s' "$response" | sed '1,/^\r$/d')

[ "$http_code" = "200" ] || { echo "FAIL: expected 200, got $http_code"; exit 1; }

printf '%s\n' "$headers" | grep -qi "x-bytetaper-pagination-applied: true" \
  || { echo "FAIL: x-bytetaper-pagination-applied header missing or not true"; exit 1; }

printf '%s\n' "$headers" | grep -qi "x-bytetaper-pagination-reason: missing_limit" \
  || { echo "FAIL: x-bytetaper-pagination-reason not missing_limit"; exit 1; }

printf '%s\n' "$body" | grep -q '"received_limit":"50"' \
  || { echo "FAIL: upstream did not receive limit=50; body: $body"; exit 1; }

# --- Scenario B: valid limit → no injection ---

echo "Scenario B: Testing valid limit passthrough..."
response2=$(curl -s -D - "${BASE}/orders?limit=100")
headers2=$(printf '%s' "$response2" | sed '/^\r$/q')

if printf '%s\n' "$headers2" | grep -qi "x-bytetaper-pagination-applied: true"; then
  echo "FAIL: pagination-applied should be absent for valid limit"
  exit 1
fi

echo "PASS: pagination default limit injection verified"
