#!/bin/bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -e

ENVOY_URL="${ENVOY_URL:-http://envoy:10000}"

echo "Scenario 1: Large Response (2KB) with Accept-Encoding: gzip"
# Should be compressed by Envoy and marked as candidate by ByteTaper
RESPONSE_HEADERS=$(curl -s -o /dev/null -D - -H "Accept-Encoding: gzip" "${ENVOY_URL}/api/v1/large")

if echo "$RESPONSE_HEADERS" | grep -qi "content-encoding: gzip"; then
    echo "PASS: Content-Encoding: gzip found"
else
    echo "FAIL: Content-Encoding: gzip missing"
    exit 1
fi

if echo "$RESPONSE_HEADERS" | grep -qi "x-bytetaper-compression-candidate: true"; then
    echo "PASS: x-bytetaper-compression-candidate: true found"
else
    echo "FAIL: x-bytetaper-compression-candidate: true missing"
    exit 1
fi

echo "Scenario 2: Small Response (512B) with Accept-Encoding: gzip"
# Should NOT be compressed by Envoy (below 1024 threshold) and NOT marked candidate by ByteTaper
RESPONSE_HEADERS=$(curl -s -o /dev/null -D - -H "Accept-Encoding: gzip" "${ENVOY_URL}/api/v1/small")

if echo "$RESPONSE_HEADERS" | grep -qi "content-encoding: gzip"; then
    echo "FAIL: Content-Encoding: gzip found on small response"
    exit 1
else
    echo "PASS: Content-Encoding: gzip missing on small response"
fi

if echo "$RESPONSE_HEADERS" | grep -qi "x-bytetaper-compression-candidate: true"; then
    echo "FAIL: x-bytetaper-compression-candidate: true found on small response"
    exit 1
else
    echo "PASS: x-bytetaper-compression-candidate: true missing on small response"
fi

echo "Scenario 3: Already Encoded Response"
# Should NOT be marked candidate by ByteTaper
RESPONSE_HEADERS=$(curl -s -o /dev/null -D - "${ENVOY_URL}/api/v1/already-encoded")

if echo "$RESPONSE_HEADERS" | grep -qi "x-bytetaper-compression-candidate: true"; then
    echo "FAIL: x-bytetaper-compression-candidate: true found on already encoded response"
    exit 1
else
    echo "PASS: x-bytetaper-compression-candidate: true missing on already encoded response"
fi

echo "ALL COMPRESSION E2E SCENARIOS PASSED"
