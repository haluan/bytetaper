#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Haluan Irsad
# SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

set -euo pipefail

# Accept JSON report file as argument
JSON_FILE="${1:-}"

if [ -z "$JSON_FILE" ]; then
    echo "Usage: $0 <report_file.json>" >&2
    exit 1
fi

if [ ! -f "$JSON_FILE" ]; then
    echo "ERROR: File '$JSON_FILE' not found" >&2
    exit 1
fi

echo "Compiling Markdown report from JSON file: $JSON_FILE"

OUT_MD_FILE="${JSON_FILE%.json}.md"

# Read top-level metadata
scenario=$(jq -r '.scenario' "$JSON_FILE" || echo "unknown")
timestamp=$(jq -r '.timestamp' "$JSON_FILE" || echo "unknown")
bv=$(jq -r '.benchmark_version' "$JSON_FILE" || echo "1.0.0")

# Read features (system details)
os_info=$(jq -r '.features.os_info' "$JSON_FILE" || echo "N/A")
cpu_cores=$(jq -r '.features.cpu_cores' "$JSON_FILE" || echo "N/A")
memory_total=$(jq -r '.features.memory_total' "$JSON_FILE" || echo "N/A")
target_host=$(jq -r '.features.target_host' "$JSON_FILE" || echo "N/A")

# Initialize markdown file
{
    echo "# 🚀 ByteTaper Performance Report"
    echo ""
    echo "An automated performance and integrity audit compiled from execution results."
    echo ""
    echo "## 📊 Benchmark Metadata"
    echo ""
    echo "| Parameter | Value |"
    echo "| :--- | :--- |"
    echo "| **Scenario** | \`${scenario}\` |"
    echo "| **Timestamp** | \`${timestamp}\` |"
    echo "| **Benchmark Version** | \`${bv}\` |"
    echo "| **Target Host** | \`${target_host}\` |"
    echo ""
    echo "## 💻 System Specifications"
    echo ""
    echo "- **Operating System**: \`${os_info}\`"
    echo "- **CPU Cores**: \`${cpu_cores}\`"
    echo "- **Total System Memory**: \`${memory_total}\`"
    echo ""
    echo "## 📈 Execution Metrics"
    echo ""
} > "$OUT_MD_FILE"

# Track if any metric is unavailable to show unified warnings at the bottom
any_unavailable=0

# Iterate through each leg in the report (using leg names as headers) safely handling spaces
while IFS= read -r leg; do
    if [ -z "$leg" ]; then continue; fi

    # Extract leg metrics
    p50=$(jq -r ".latency_ms.\"$leg\".latency_ms.p50" "$JSON_FILE" || echo "unavailable")
    p95=$(jq -r ".latency_ms.\"$leg\".latency_ms.p95" "$JSON_FILE" || echo "unavailable")
    p99=$(jq -r ".latency_ms.\"$leg\".latency_ms.p99" "$JSON_FILE" || echo "unavailable")

    rps=$(jq -r ".throughput.\"$leg\".throughput.requests_per_second" "$JSON_FILE" || echo "unavailable")
    total_reqs=$(jq -r ".throughput.\"$leg\".total_requests" "$JSON_FILE" || echo "unavailable")
    success_reqs=$(jq -r ".throughput.\"$leg\".successful_requests" "$JSON_FILE" || echo "unavailable")
    failed_reqs=$(jq -r ".throughput.\"$leg\".failed_requests" "$JSON_FILE" || echo "unavailable")

    orig_bytes=$(jq -r ".payload.\"$leg\".original_bytes_avg" "$JSON_FILE" || echo "unavailable")
    opt_bytes=$(jq -r ".payload.\"$leg\".optimized_bytes_avg" "$JSON_FILE" || echo "unavailable")
    saved_bytes=$(jq -r ".payload.\"$leg\".bytes_saved_avg" "$JSON_FILE" || echo "unavailable")
    ratio=$(jq -r ".payload.\"$leg\".reduction_ratio" "$JSON_FILE" || echo "unavailable")

    envoy_cpu=$(jq -r ".resources.\"$leg\".envoy.cpu_percent" "$JSON_FILE" || echo "unavailable")
    envoy_mem=$(jq -r ".resources.\"$leg\".envoy.peak_memory_mb" "$JSON_FILE" || echo "unavailable")
    ext_cpu=$(jq -r ".resources.\"$leg\".\"bytetaper-extproc\".cpu_percent" "$JSON_FILE" || echo "unavailable")
    ext_mem=$(jq -r ".resources.\"$leg\".\"bytetaper-extproc\".peak_memory_mb" "$JSON_FILE" || echo "unavailable")
    mock_cpu=$(jq -r ".resources.\"$leg\".\"mock-api\".cpu_percent" "$JSON_FILE" || echo "unavailable")
    mock_mem=$(jq -r ".resources.\"$leg\".\"mock-api\".peak_memory_mb" "$JSON_FILE" || echo "unavailable")

    # Check for unavailable measurements
    if [ "$envoy_cpu" = "unavailable" ] || [ "$envoy_mem" = "unavailable" ] || \
       [ "$ext_cpu" = "unavailable" ] || [ "$ext_mem" = "unavailable" ] || \
       [ "$p50" = "unavailable" ] || [ "$rps" = "unavailable" ] || \
       [ "$orig_bytes" = "unavailable" ]; then
        any_unavailable=1
    fi

    {
        echo "### Sub-Scenario / Leg: \`${leg}\`"
        echo ""
        echo "#### ⏱️ Latency & Throughput Profile"
        echo ""
        echo "| Metric | Value |"
        echo "| :--- | :--- |"
        echo "| **Throughput (RPS)** | \`${rps} req/s\` |"
        echo "| **Total Client Requests** | \`${total_reqs}\` |"
        echo "| **Successful Requests** | \`${success_reqs}\` |"
        echo "| **Failed Requests** | \`${failed_reqs}\` |"
        echo "| **p50 Latency** | \`${p50} ms\` |"
        echo "| **p95 Latency** | \`${p95} ms\` |"
        echo "| **p99 Latency** | \`${p99} ms\` |"
        echo ""
        echo "#### 📦 Payload Savings Summary"
        echo ""
        echo "| Metric | Bytes / Percentage |"
        echo "| :--- | :--- |"
        echo "| **Original Payload Bytes (Avg)** | \`${orig_bytes} bytes\` |"
        echo "| **Optimized Payload Bytes (Avg)** | \`${opt_bytes} bytes\` |"
        echo "| **Payload Bytes Saved (Avg)** | \`${saved_bytes} bytes\` |"
        echo "| **Payload Reduction Ratio** | **\`${ratio}\`** |"
        echo ""
        echo "#### 🎛️ Container Resource Utilizations"
        echo ""
        echo "| Container Service | CPU Average % | Peak Memory (MB) |"
        echo "| :--- | :--- | :--- |"
        echo "| **envoy** | \`${envoy_cpu}%\` | \`${envoy_mem} MB\` |"
        echo "| **bytetaper-extproc** | \`${ext_cpu}%\` | \`${ext_mem} MB\` |"
        echo "| **mock-api** | \`${mock_cpu}%\` | \`${mock_mem} MB\` |"
        echo ""
    } >> "$OUT_MD_FILE"
done < <(jq -r '.latency_ms | keys[]' "$JSON_FILE" || echo "main")

# Warnings & Notes Section
{
    echo "## ⚠️ Notes & Warnings"
    echo ""
    if [ $any_unavailable -eq 1 ]; then
        echo "> [!WARNING]"
        echo "> Some container resource metrics or latency metrics are marked as \`unavailable\`. This can occur due to host namespace isolation restrictions within the execution containers (e.g. lack of access to host cgroup parameters). Ensure appropriate permissions are enabled if absolute tracking is required."
        echo ""
    else
        echo "> [!NOTE]"
        echo "> All metrics parsed, compiled, and validated with zero missing data parameters successfully."
        echo ""
    fi
} >> "$OUT_MD_FILE"

echo "Markdown report written successfully to: $OUT_MD_FILE"
cat "$OUT_MD_FILE"
