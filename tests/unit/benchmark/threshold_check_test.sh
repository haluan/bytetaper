#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

assert_exit_code() {
    local expected=$1
    local cmd=$2
    local msg=$3

    set +e
    eval "$cmd" > /dev/null 2>&1
    local actual=$?
    set -e

    if [ "$expected" -ne "$actual" ]; then
        echo "FAIL: $msg" >&2
        echo "  Expected Exit Code: $expected" >&2
        echo "  Actual Exit Code:   $actual" >&2
        exit 1
    else
        echo "PASS: $msg (Exit Code: $expected)"
    fi
}

echo "=== Running Performance Threshold Checker Unit Tests ==="

# Temporary directories for configs and test inputs
MOCK_JSON_PASS=$(mktemp)
MOCK_JSON_FAIL=$(mktemp)
MOCK_JSON_UNAVAIL=$(mktemp)

# 1. Create passing report JSON
cat << 'EOF' > "$MOCK_JSON_PASS"
{
  "benchmark_version": "1.0.0",
  "scenario": "bytetaper_observe",
  "timestamp": "Wed May  6 14:17:16 UTC 2026",
  "latency_ms": {
    "main": {
      "latency_ms": {
        "p50": 125.0,
        "p95": 145.5,
        "p99": 155.0
      }
    }
  },
  "throughput": {
    "main": {
      "throughput": {
        "requests_per_second": 84.0,
        "total_requests": 1000,
        "successful_requests": 1000,
        "failed_requests": 0
      },
      "total_requests": 1000,
      "successful_requests": 1000,
      "failed_requests": 0
    }
  },
  "payload": {
    "main": {
      "original_bytes_avg": 1000,
      "optimized_bytes_avg": 1000,
      "bytes_saved_avg": 0,
      "reduction_ratio": "0.00%"
    }
  }
}
EOF

# 2. Create failing report JSON (P95 is 175.5, exceeding the 160.0 threshold for bytetaper_observe)
cat << 'EOF' > "$MOCK_JSON_FAIL"
{
  "benchmark_version": "1.0.0",
  "scenario": "bytetaper_observe",
  "timestamp": "Wed May  6 14:17:16 UTC 2026",
  "latency_ms": {
    "main": {
      "latency_ms": {
        "p50": 125.0,
        "p95": 175.5,
        "p99": 185.0
      }
    }
  },
  "throughput": {
    "main": {
      "throughput": {
        "requests_per_second": 84.0,
        "total_requests": 1000,
        "successful_requests": 1000,
        "failed_requests": 0
      },
      "total_requests": 1000,
      "successful_requests": 1000,
      "failed_requests": 0
    }
  },
  "payload": {
    "main": {
      "original_bytes_avg": 1000,
      "optimized_bytes_avg": 1000,
      "bytes_saved_avg": 0,
      "reduction_ratio": "0.00%"
    }
  }
}
EOF

# 3. Create report with unavailable metrics
cat << 'EOF' > "$MOCK_JSON_UNAVAIL"
{
  "benchmark_version": "1.0.0",
  "scenario": "bytetaper_observe",
  "timestamp": "Wed May  6 14:17:16 UTC 2026",
  "latency_ms": {
    "main": {
      "latency_ms": {
        "p50": "unavailable",
        "p95": "unavailable",
        "p99": "unavailable"
      }
    }
  },
  "throughput": {
    "main": {
      "total_requests": "unavailable",
      "successful_requests": "unavailable",
      "failed_requests": "unavailable"
    }
  },
  "payload": {
    "main": {
      "reduction_ratio": "unavailable"
    }
  }
}
EOF

# Execute threshold validations and check exit codes
assert_exit_code 0 "./benchmarks/report/check_thresholds.sh $MOCK_JSON_PASS" "Verify passing report exits zero"
assert_exit_code 1 "./benchmarks/report/check_thresholds.sh $MOCK_JSON_FAIL" "Verify failing report exits non-zero"
assert_exit_code 0 "./benchmarks/report/check_thresholds.sh $MOCK_JSON_UNAVAIL" "Verify report with unavailable parameters skips smoothly and exits zero"

# Cleanup
rm -f "$MOCK_JSON_PASS" "$MOCK_JSON_FAIL" "$MOCK_JSON_UNAVAIL"

echo "=== All Performance Threshold Checker Unit Tests Passed ==="
exit 0
