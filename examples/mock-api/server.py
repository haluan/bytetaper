#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial


import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlsplit

HOST = "0.0.0.0"
PORT = 8080
PATH = "/api/v1/oversized"
MIN_BYTES = 1_048_576
TARGET_BYTES = 1_200_000


def build_payload_bytes() -> bytes:
    base_object = {
        "service": "mock-api",
        "scenario": "oversized-json",
        "sentinel": "bt-001",
        "version": 1,
        "payload": "",
    }

    body = json.dumps(base_object, separators=(",", ":"), sort_keys=True)
    base_size = len(body.encode("utf-8"))
    if base_size > TARGET_BYTES:
        raise RuntimeError("base payload unexpectedly exceeds target")

    filler_len = TARGET_BYTES - base_size
    base_object["payload"] = "x" * filler_len
    payload = json.dumps(base_object, separators=(",", ":"), sort_keys=True).encode("utf-8")

    if len(payload) <= MIN_BYTES:
        raise RuntimeError("payload is not oversized")

    return payload


PAYLOAD = build_payload_bytes()


class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if urlsplit(self.path).path != PATH:
            self.send_response(404)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error":"not_found"}')
            return

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(PAYLOAD)))
        self.end_headers()
        self.wfile.write(PAYLOAD)

    def log_message(self, fmt: str, *args) -> None:
        return


if __name__ == "__main__":
    server = HTTPServer((HOST, PORT), Handler)
    server.serve_forever()
