#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

import urllib.request
import os
import sys

BASE_URL = os.environ.get("BYTETAPER_TEST_BASE_URL", "http://localhost:10000")
MOCK_COUNTER_URL = os.environ.get("BYTETAPER_MOCK_COUNTER_URL", "http://localhost:9000/call-count")

def get_url(url):
    with urllib.request.urlopen(url) as response:
        return response.read().decode('utf-8'), response.getcode(), response.info()

print("Starting cache hit immediate response test...")

# Warm the cache
print("Warming cache...")
try:
    _, code, _ = get_url(f"{BASE_URL}/api/v1/cached/items?fields=id")
    if code != 200:
        print(f"FAIL: first request got {code}")
        sys.exit(1)
except Exception as e:
    print(f"FAIL: first request failed: {e}")
    sys.exit(1)

# Get current counter
try:
    counter_before, _, _ = get_url(MOCK_COUNTER_URL)
    print(f"Mock counter before: {counter_before}")
except Exception as e:
    print(f"FAIL: failed to get counter: {e}")
    sys.exit(1)

# Second request should hit cache
print("Performing second request (expecting cache hit)...")
try:
    _, code, info = get_url(f"{BASE_URL}/api/v1/cached/items?fields=id")
    if "x-bytetaper-cached-response" not in str(info).lower():
        print("FAIL: second request did not return cache hit header")
        print(info)
        sys.exit(1)
except Exception as e:
    print(f"FAIL: second request failed: {e}")
    sys.exit(1)

# Verify mock counter has NOT increased
try:
    counter_after, _, _ = get_url(MOCK_COUNTER_URL)
    print(f"Mock counter after: {counter_after}")
except Exception as e:
    print(f"FAIL: failed to get counter after: {e}")
    sys.exit(1)

if counter_before != counter_after:
    print(f"FAIL: mock counter changed from {counter_before} to {counter_after}")
    sys.exit(1)

print("PASS: cache hit returned immediate response; mock counter unchanged")
