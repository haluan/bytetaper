#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial


import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

URL_FILTERED = "http://envoy:10000/api/v1/oversized?fields=service,scenario"
URL_UNFILTERED = "http://envoy:10000/api/v1/oversized"
ENVOY_ADMIN_URL = "http://envoy:9901/stats"
MIN_BYTES = 1_048_576
TARGET_BYTES = 1_200_000
EXPECTED_SENTINEL = "bt-001"
TIMEOUT_SECONDS = 30


def fail(message: str, code: int) -> int:
    sys.stderr.write(message + "\n")
    return code


def get_header_case_insensitive(headers, key: str) -> str:
    target = key.lower()
    for header_key, header_value in headers.items():
        if header_key.lower() == target:
            return header_value
    return ""


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


def build_expected_unfiltered_payload_bytes() -> bytes:
    base_object = {
        "service": "mock-api",
        "scenario": "oversized-json",
        "sentinel": EXPECTED_SENTINEL,
        "version": 1,
        "payload": "",
    }

    base_body = json.dumps(base_object, separators=(",", ":"), sort_keys=True)
    base_size = len(base_body.encode("utf-8"))
    if base_size > TARGET_BYTES:
        raise RuntimeError("base payload unexpectedly exceeds target")

    filler_length = TARGET_BYTES - base_size
    base_object["payload"] = "x" * filler_length
    payload = json.dumps(base_object, separators=(",", ":"), sort_keys=True).encode("utf-8")

    if len(payload) <= MIN_BYTES:
        raise RuntimeError("expected payload is not oversized")

    return payload


def main() -> int:
    sent_counter = "http.ingress_http.ext_proc.stream_msgs_sent"
    recv_counter = "http.ingress_http.ext_proc.stream_msgs_received"
    pre_sent = read_counter(sent_counter)
    pre_recv = read_counter(recv_counter)

    deadline = time.time() + TIMEOUT_SECONDS
    while True:
        try:
            with urllib.request.urlopen(URL_FILTERED, timeout=3) as filtered_response:
                filtered_status = filtered_response.status
                filtered_content_type = filtered_response.headers.get("Content-Type", "")
                filtered_body = filtered_response.read()

            if filtered_status != 200:
                return fail(f"expected status 200, got {filtered_status}", 2)
            
            if (
                get_header_case_insensitive(
                    filtered_response.headers, "x-bytetaper-extproc-response-body"
                )
                != "true"
            ):
                return fail(
                    "missing or incorrect x-bytetaper-extproc-response-body header in response", 10
                )

            if "application/json" not in filtered_content_type.lower():
                return fail(
                    f"expected application/json content type, got: {filtered_content_type}", 3
                )

            try:
                filtered_payload = json.loads(filtered_body.decode("utf-8"))
            except Exception as exc:
                return fail(f"response body is not valid json: {exc}", 5)

            expected_keys = {"service", "scenario"}
            actual_keys = set(filtered_payload.keys())
            if actual_keys != expected_keys:
                return fail(f"expected exactly keys {expected_keys}, but got {actual_keys}", 11)
            
            if filtered_payload.get("service") != "mock-api":
                return fail("missing or incorrect service marker in payload", 6)

            if filtered_payload.get("scenario") != "oversized-json":
                return fail("missing or incorrect scenario marker in payload", 7)

            with urllib.request.urlopen(URL_UNFILTERED, timeout=3) as unfiltered_response:
                unfiltered_status = unfiltered_response.status
                unfiltered_content_type = unfiltered_response.headers.get("Content-Type", "")
                unfiltered_body = unfiltered_response.read()

            if unfiltered_status != 200:
                return fail(f"expected baseline status 200, got {unfiltered_status}", 12)
            
            if (
                get_header_case_insensitive(
                    unfiltered_response.headers, "x-bytetaper-extproc-response-body"
                )
                != "true"
            ):
                return fail(
                    "missing or incorrect x-bytetaper-extproc-response-body header in unfiltered response",
                    17,
                )

            if "application/json" not in unfiltered_content_type.lower():
                return fail(
                    f"expected baseline application/json content type, got: {unfiltered_content_type}",
                    13,
                )

            if len(unfiltered_body) <= MIN_BYTES:
                return fail(
                    f"expected oversized baseline body > {MIN_BYTES} bytes, got {len(unfiltered_body)} bytes",
                    4,
                )
            
            expected_unfiltered_body = build_expected_unfiltered_payload_bytes()
            if unfiltered_body != expected_unfiltered_body:
                return fail("unfiltered body does not exactly match deterministic mock payload", 18)

            if len(filtered_body) >= len(unfiltered_body):
                return fail(
                    f"expected filtered body smaller than baseline body ({len(filtered_body)} >= {len(unfiltered_body)})",
                    16,
                )

            post_sent = read_counter(sent_counter)
            post_recv = read_counter(recv_counter)
            if (post_sent - pre_sent) < 3 or (post_recv - pre_recv) < 3:
                return fail("ext_proc stream counters did not increase as expected", 9)

            print("SUCCESS: filtered payload received through envoy ext_proc path")
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
