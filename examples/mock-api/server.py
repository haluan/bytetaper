#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlsplit

HOST = "0.0.0.0"
PORT = 8080

# Global call counter
g_call_count = 0

def build_payload(size=1024, scenario="default", sentinel="bt-001", version=1):
    base_object = {
        "service": "mock-api",
        "scenario": scenario,
        "sentinel": sentinel,
        "version": version,
        "payload": "x" * size,
    }
    # Use separators to match the test's json.dumps output
    return json.dumps(base_object, separators=(",", ":"), sort_keys=True).encode("utf-8")

DEFAULT_PAYLOAD = build_payload(1024)

class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        global g_call_count
        path = urlsplit(self.path).path

        if path == "/call-count":
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(str(g_call_count).encode("utf-8"))
            return

        # Increment counter for any other GET request
        g_call_count += 1

        if path == "/api/v1/oversized":
            # Exact size matching logic from oversized_envoy_assert.py
            base_object = {
                "service": "mock-api",
                "scenario": "oversized-json",
                "sentinel": "bt-001",
                "version": 1,
                "payload": "",
            }
            base_body = json.dumps(base_object, separators=(",", ":"), sort_keys=True)
            base_size = len(base_body.encode("utf-8"))
            filler_length = 1200000 - base_size
            payload = build_payload(filler_length, scenario="oversized-json")
            
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path.startswith("/api/v1/"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(DEFAULT_PAYLOAD)))
            self.end_headers()
            self.wfile.write(DEFAULT_PAYLOAD)
            return

        self.send_response(404)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"error":"not_found"}')

    def log_message(self, fmt: str, *args) -> None:
        return

if __name__ == "__main__":
    server = HTTPServer((HOST, PORT), Handler)
    print(f"Mock API listening on {HOST}:{PORT}")
    server.serve_forever()
