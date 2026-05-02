#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

import os
import sys
import urllib.request
import uuid

RUN_ID = uuid.uuid4().hex[:8]

BASE_URL = os.environ.get("BYTETAPER_TEST_BASE_URL", "http://envoy:10000")
COUNTER_URL = os.environ.get("BYTETAPER_MOCK_COUNTER_URL", "http://mock-api:8080/call-count")
RESET_URL = os.environ.get("BYTETAPER_MOCK_RESET_URL", "http://mock-api:8080/reset-count")

PATH_A = f"{BASE_URL}/api/v1/e2e/items-{RUN_ID}-a?fields=service,version"
PATH_B = f"{BASE_URL}/api/v1/e2e/items-{RUN_ID}-b?fields=service,version"

def counter():
    return int(urllib.request.urlopen(COUNTER_URL).read().strip())

def get(url):
    resp = urllib.request.urlopen(url)
    hdrs = {k.lower(): v for k, v in resp.getheaders()}
    body = resp.read()
    return hdrs, body

def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)

print("Starting Cache E2E Test...")

# Reset counter for clean test run
urllib.request.urlopen(urllib.request.Request(RESET_URL, method="GET"))

# Step 1: First request — must miss; upstream must be called
print("Step 1: First request (expecting miss)...")
c0 = counter()
hdrs1, body1 = get(PATH_A)
c1 = counter()
if hdrs1.get("x-bytetaper-cached-response") == "true":
    fail("Step 1: expected cache miss, got hit")
if c1 != c0 + 1:
    fail(f"Step 1: counter should be {c0+1}, got {c1}")

# Step 2: Second request — must hit L1; upstream must NOT be called
print("Step 2: Second request (expecting L1 hit)...")
hdrs2, body2 = get(PATH_A)
c2 = counter()
if hdrs2.get("x-bytetaper-cached-response") != "true":
    fail("Step 2: expected cache hit, got miss")
if hdrs2.get("x-bytetaper-cache-layer") != "L1":
    fail(f"Step 2: expected L1 hit, got layer={hdrs2.get('x-bytetaper-cache-layer')}")
if c2 != c1:
    fail(f"Step 2: counter should be {c1} (no upstream call), got {c2}")

# Step 3: Different keys evict PATH_A from L1 (kL1SlotCount=16)
print("Step 3: Evicting L1 with multiple keys...")
EVICT_COUNT = 16
for i in range(EVICT_COUNT):
    evict_path = f"{BASE_URL}/api/v1/e2e/evict-{RUN_ID}-{i}?fields=service,version"
    get(evict_path)
c3 = counter()
# We expect EVICT_COUNT more calls to the upstream for the eviction keys
if c3 != c2 + EVICT_COUNT:
    fail(f"Step 3: counter should be {c2+EVICT_COUNT} after evictions, got {c3}")

# Step 4: PATH_A again — L1 miss (evicted), L2 hit; upstream must NOT be called; promote to L1
print("Step 4: Requesting first key (expecting L2 hit after eviction)...")
hdrs4, body4 = get(PATH_A)
c4 = counter()
if hdrs4.get("x-bytetaper-cached-response") != "true":
    fail("Step 4: expected L2 hit, got miss — L2 did not persist or promote")
if hdrs4.get("x-bytetaper-cache-layer") != "L2":
    fail(f"Step 4: expected L2 layer, got {hdrs4.get('x-bytetaper-cache-layer')}")
if c4 != c3:
    fail(f"Step 4: counter should be {c3} (L2 served), got {c4}")

# Step 5: Same path again — should now hit L1 (promoted from L2 in step 4)
print("Step 5: Requesting first key (expecting L1 hit after promotion)...")
hdrs5, _ = get(PATH_A)
if hdrs5.get("x-bytetaper-cache-layer") != "L1":
    fail(f"Step 5: expected L1 after promotion, got {hdrs5.get('x-bytetaper-cache-layer')}")

print("PASS: L1 miss → store → L1 hit → L1 evict → L2 hit → L1 promoted")
