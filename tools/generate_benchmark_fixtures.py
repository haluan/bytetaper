#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

import os
import json

def main():
    fixtures_dir = "/Users/haluan.irsad/Documents/go-work/code/bytetaper/benchmarks/fixtures"
    os.makedirs(fixtures_dir, exist_ok=True)

    # 1. small-json.json (~512 bytes)
    small_obj = {
        "id": "small-fixture-001",
        "name": "Small Fixture Item",
        "description": "This is a small JSON fixture of approximately 512 bytes.",
        "type": "test_fixture",
        "active": True,
        "metadata": {
            "version": "1.0.0",
            "tags": ["bench", "fixture", "small"],
            "checksum": "abc123xyz"
        },
        "padding": "x" * 262
    }
    small_json = json.dumps(small_obj, separators=(",", ":"), sort_keys=True)

    # 2. medium-json.json (~8 KB)
    medium_obj = {
        "id": "medium-fixture-001",
        "name": "Medium Fixture Item",
        "description": "This is a medium JSON fixture of approximately 8 KB.",
        "type": "test_fixture",
        "active": True,
        "metadata": {
            "version": "1.0.0",
            "tags": ["bench", "fixture", "medium"],
            "checksum": "medium-abc123xyz"
        },
        "padding": "x" * 7800
    }
    medium_json = json.dumps(medium_obj, separators=(",", ":"), sort_keys=True)

    # 3. large-json.json (~128 KB)
    large_obj = {
        "id": "large-fixture-001",
        "name": "Large Fixture Item",
        "description": "This is a large JSON fixture of approximately 128 KB.",
        "type": "test_fixture",
        "active": True,
        "metadata": {
            "version": "1.0.0",
            "tags": ["bench", "fixture", "large"],
            "checksum": "large-abc123xyz"
        },
        "padding": "x" * 127000
    }
    large_json = json.dumps(large_obj, separators=(",", ":"), sort_keys=True)

    # 4. huge-json.json (~1 MB)
    huge_obj = {
        "id": "huge-fixture-001",
        "name": "Huge Fixture Item",
        "description": "This is a huge JSON fixture of approximately 1 MB.",
        "type": "test_fixture",
        "active": True,
        "metadata": {
            "version": "1.0.0",
            "tags": ["bench", "fixture", "huge"],
            "checksum": "huge-abc123xyz"
        },
        "padding": "x" * 1047000
    }
    huge_json = json.dumps(huge_obj, separators=(",", ":"), sort_keys=True)

    # 5. products-by-id.json (deterministic object template for /products/:id)
    products_obj = {
        "product_id": "prod-12345",
        "sku": "PROD-SKU-98765",
        "name": "Deterministic Product Name",
        "price": 129.99,
        "currency": "USD",
        "in_stock": True,
        "specifications": {
            "weight": "1.2kg",
            "dimensions": "10x20x30cm",
            "color": "Midnight Black"
        }
    }
    products_json = json.dumps(products_obj, separators=(",", ":"), sort_keys=True)

    # 6. orders-list.json (deterministic list template used by /orders response body shape)
    orders_obj = {
        "data": [
            {
                "order_id": "order-101",
                "customer_id": "cust-501",
                "total": 45.99,
                "status": "shipped"
            },
            {
                "order_id": "order-102",
                "customer_id": "cust-502",
                "total": 120.50,
                "status": "pending"
            }
        ]
    }
    orders_json = json.dumps(orders_obj, separators=(",", ":"), sort_keys=True)

    with open(os.path.join(fixtures_dir, "small-json.json"), "w") as f:
        f.write(small_json)
    with open(os.path.join(fixtures_dir, "medium-json.json"), "w") as f:
        f.write(medium_json)
    with open(os.path.join(fixtures_dir, "large-json.json"), "w") as f:
        f.write(large_json)
    with open(os.path.join(fixtures_dir, "huge-json.json"), "w") as f:
        f.write(huge_json)
    with open(os.path.join(fixtures_dir, "products-by-id.json"), "w") as f:
        f.write(products_json)
    with open(os.path.join(fixtures_dir, "orders-list.json"), "w") as f:
        f.write(orders_json)

    print("Fixtures generated successfully.")

if __name__ == "__main__":
    main()
