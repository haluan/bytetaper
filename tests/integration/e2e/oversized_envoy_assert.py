#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial


import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

URL = "http://envoy:10000/api/v1/oversized"
ENVOY_ADMIN_URL = "http://envoy:9901/stats"
MIN_BYTES = 1_048_576
EXPECTED_SENTINEL = "bt-001"
TIMEOUT_SECONDS = 30


def fail(message: str, code: int) -> int:
    sys.stderr.write(message + "\n")
    return code


def read_counter(name: str) -> int:
    query = urllib.parse.urlencode({"filter": f"^{name}$"})
    with urllib.request.urlopen(f"{ENVOY_ADMIN_URL}?{query}", timeout=3) as response:
        body = response.read().decode("utf-8")

    for line in body.splitlines():
        if not line.startswith(f"{name}:"):
            continue
        _, raw_value = line.split(":", 1)
        return int(raw_value.strip())

    return 0


def main() -> int:
    sent_counter = "http.ingress_http.ext_proc.stream_msgs_sent"
    recv_counter = "http.ingress_http.ext_proc.stream_msgs_received"
    pre_sent = read_counter(sent_counter)
    pre_recv = read_counter(recv_counter)

    deadline = time.time() + TIMEOUT_SECONDS
    while True:
        try:
            with urllib.request.urlopen(URL, timeout=3) as response:
                status_code = response.status
                content_type = response.headers.get("Content-Type", "")
                body = response.read()

            if response.headers.get("x-bytetaper-extproc-response-body") != "true":
                return fail(
                    "missing or incorrect x-bytetaper-extproc-response-body header in response", 10
                )

            if status_code != 200:
                return fail(f"expected status 200, got {status_code}", 2)

            if "application/json" not in content_type.lower():
                return fail(f"expected application/json content type, got: {content_type}", 3)

            if len(body) <= MIN_BYTES:
                return fail(
                    f"expected oversized body > {MIN_BYTES} bytes, got {len(body)} bytes", 4
                )

            try:
                payload = json.loads(body.decode("utf-8"))
            except Exception as exc:
                return fail(f"response body is not valid json: {exc}", 5)

            if payload.get("sentinel") != EXPECTED_SENTINEL:
                return fail(
                    "missing or incorrect sentinel in payload", 6
                )

            if payload.get("scenario") != "oversized-json":
                return fail(
                    "missing or incorrect scenario marker in payload", 7
                )

            post_sent = read_counter(sent_counter)
            post_recv = read_counter(recv_counter)
            if (post_sent - pre_sent) < 3 or (post_recv - pre_recv) < 3:
                return fail("ext_proc response-body buffered callback proof did not execute", 9)

            print("SUCCESS: oversized payload received through envoy")
            return 0
        except urllib.error.HTTPError as exc:
            if exc.code == 503 and time.time() < deadline:
                time.sleep(1)
                continue
            return fail(f"expected status 200, got {exc.code}", 2)
        except (urllib.error.URLError, TimeoutError):
            if time.time() >= deadline:
                return fail("timed out waiting for envoy demo endpoint", 8)
            time.sleep(1)


if __name__ == "__main__":
    sys.exit(main())
