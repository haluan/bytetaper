#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Allow overriding base URL for testing flexibility
MOCK_API_URL="${BYTETAPER_MOCK_API_URL:-http://mock-api:8080}"

echo "=== Running Benchmark Fixture Integration Test ==="
echo "Target Mock API URL: ${MOCK_API_URL}"

# Helper to fetch endpoint
fetch_endpoint() {
    local path="$1"
    curl -sSf "${MOCK_API_URL}${path}"
}

# Helper to verify size in range
verify_size_in_range() {
    local name="$1"
    local size="$2"
    local min="$3"
    local max="$4"
    echo "  - ${name} size: ${size} bytes (expected range: [${min}, ${max}])"
    if [ "${size}" -lt "${min}" ] || [ "${size}" -gt "${max}" ]; then
        echo "Error: ${name} size ${size} is out of expected range [${min}, ${max}]!"
        exit 1
    fi
}

echo "1. Checking reachability and repeated-fetch determinism..."
for endpoint in "/small-json" "/medium-json" "/large-json" "/huge-json" "/products/123" "/orders"; do
    echo "Testing endpoint ${endpoint}..."
    res1=$(fetch_endpoint "${endpoint}")
    res2=$(fetch_endpoint "${endpoint}")
    if [ "${res1}" != "${res2}" ]; then
        echo "Error: Non-deterministic response detected on ${endpoint}!"
        exit 1
    fi
done
echo "All endpoints are reachable and fully deterministic."

echo "2. Validating approximate size bands..."
small_size=$(fetch_endpoint "/small-json" | wc -c)
verify_size_in_range "small-json" "${small_size}" 400 600

medium_size=$(fetch_endpoint "/medium-json" | wc -c)
verify_size_in_range "medium-json" "${medium_size}" 7000 9000

large_size=$(fetch_endpoint "/large-json" | wc -c)
verify_size_in_range "large-json" "${large_size}" 120000 135000

huge_size=$(fetch_endpoint "/huge-json" | wc -c)
verify_size_in_range "huge-json" "${huge_size}" 1000000 1100000

echo "Size bands are fully compliant."

echo "3. Validating payload schemas and content structures..."

# Orders list coverage
orders_res=$(fetch_endpoint "/orders")
if [[ "${orders_res}" != *'"data":['* ]]; then
    echo "Error: /orders payload does not have expected list-style 'data' field!"
    exit 1
fi
echo "  - /orders contains list-style structure."

# Products object coverage
products_res=$(fetch_endpoint "/products/123")
if [[ "${products_res}" != *'"product_id":'* ]]; then
    echo "Error: /products/:id payload does not have expected object 'product_id' field!"
    exit 1
fi
echo "  - /products/:id contains object structure."

echo "=== Benchmark Fixture Integration Test PASSED ==="
