#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

import time
import json
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlsplit

HOST = "0.0.0.0"
PORT = 8080

# Global call counter
g_call_count = 0

# Determine fixtures directory
FIXTURES_DIR = os.environ.get("BYTETAPER_FIXTURES_DIR", "/workspace/benchmarks/fixtures")
if not os.path.isdir(FIXTURES_DIR):
    FIXTURES_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "benchmarks", "fixtures"))

# Cache of fixtures loaded at startup
FIXTURES = {}
for name in ["small-json", "medium-json", "large-json", "huge-json", "products-by-id", "orders-list"]:
    path = os.path.join(FIXTURES_DIR, f"{name}.json")
    try:
        with open(path, "rb") as f:
            FIXTURES[name] = f.read()
    except Exception as e:
        print(f"Warning: Failed to load fixture {name} from {path}: {e}")
        FIXTURES[name] = b'{"error":"fixture_not_found"}'

def build_payload(size=1024, scenario="default", sentinel="bt-001", version=1):
    base_object = {
        "service": "mock-api",
        "scenario": scenario,
        "sentinel": sentinel,
        "version": version,
        "payload": "x" * size,
    }
    return json.dumps(base_object, separators=(",", ":"), sort_keys=True).encode("utf-8")

DEFAULT_PAYLOAD = build_payload(1024)

class Handler(BaseHTTPRequestHandler):
    def do_HEAD(self) -> None:
        self.do_GET()

    def do_POST(self) -> None:
        self.do_GET()

    def do_GET(self) -> None:
        global g_call_count
        path = urlsplit(self.path).path

        if path == "/call-count":
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(str(g_call_count).encode("utf-8"))
            return

        if path == "/reset-count":
            g_call_count = 0
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(b"ok")
            return

        # Artificial delay for load testing coalescing
        if "/slow/" in path:
            time.sleep(0.1) # 100ms delay to trigger wait window timeout (50ms)
        elif "/fast/" in path:
            time.sleep(0.02) # 20ms delay to ensure concurrent requests overlap in ext-proc

        if path == "/orders":
            g_call_count += 1
            query = urlsplit(self.path).query
            params = {k: v for k, v in [q.split("=") for q in query.split("&") if "=" in q]}
            limit = params.get("limit", None)
            
            try:
                data_dict = json.loads(FIXTURES["orders-list"])
            except Exception:
                data_dict = {"data": []}
            data_dict["received_limit"] = limit
            
            response_body = json.dumps(data_dict, separators=(",", ":"), sort_keys=True).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(response_body)))
            self.end_headers()
            self.wfile.write(response_body)
            return

        # Serve benchmark endpoints
        if path == "/small-json":
            g_call_count += 1
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(FIXTURES["small-json"])))
            self.end_headers()
            self.wfile.write(FIXTURES["small-json"])
            return

        if path == "/medium-json":
            g_call_count += 1
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(FIXTURES["medium-json"])))
            self.end_headers()
            self.wfile.write(FIXTURES["medium-json"])
            return

        if path == "/large-json":
            g_call_count += 1
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(FIXTURES["large-json"])))
            self.end_headers()
            self.wfile.write(FIXTURES["large-json"])
            return

        if path == "/huge-json":
            g_call_count += 1
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(FIXTURES["huge-json"])))
            self.end_headers()
            self.wfile.write(FIXTURES["huge-json"])
            return

        if path.startswith("/products/"):
            g_call_count += 1
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(FIXTURES["products-by-id"])))
            self.end_headers()
            self.wfile.write(FIXTURES["products-by-id"])
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

        if path == "/api/v1/large":
            payload = build_payload(2048, scenario="large-json")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/api/v1/small":
            payload = build_payload(512, scenario="small-json")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/api/v1/already-encoded":
            payload = b"fake-gzip-data"
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Encoding", "gzip")
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
