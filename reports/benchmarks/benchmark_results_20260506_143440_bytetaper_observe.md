# 🚀 ByteTaper Performance Report

An automated performance and integrity audit compiled from execution results.

## 📊 Benchmark Metadata

| Parameter | Value |
| :--- | :--- |
| **Scenario** | `bytetaper_observe` |
| **Timestamp** | `Wed May  6 14:34:40 UTC 2026` |
| **Benchmark Version** | `1.0.0` |
| **Target Host** | `http://envoy-bytetaper-observe:10000/api/v1/cached/fast/bench` |

## 💻 System Specifications

- **Operating System**: `Linux a08c9d2a3bc7 6.11.3-200.fc40.aarch64 aarch64 Linux`
- **CPU Cores**: `2`
- **Total System Memory**: `3617.73 MB`

## 📈 Execution Metrics

### Sub-Scenario / Leg: `main`

#### ⏱️ Latency & Throughput Profile

| Metric | Value |
| :--- | :--- |
| **Throughput (RPS)** | `84.24 req/s` |
| **Total Client Requests** | `848` |
| **Successful Requests** | `848` |
| **Failed Requests** | `0` |
| **p50 Latency** | `125.376 ms` |
| **p95 Latency** | `133.932 ms` |
| **p99 Latency** | `146.937 ms` |

#### 📦 Payload Savings Summary

| Metric | Bytes / Percentage |
| :--- | :--- |
| **Original Payload Bytes (Avg)** | `1112 bytes` |
| **Optimized Payload Bytes (Avg)** | `1112 bytes` |
| **Payload Bytes Saved (Avg)** | `0 bytes` |
| **Payload Reduction Ratio** | **`0.00%`** |

#### 🎛️ Container Resource Utilizations

| Container Service | CPU Average % | Peak Memory (MB) |
| :--- | :--- | :--- |
| **envoy** | `unavailable%` | `unavailable MB` |
| **bytetaper-extproc** | `unavailable%` | `unavailable MB` |
| **mock-api** | `unavailable%` | `unavailable MB` |

## ⚠️ Notes & Warnings

> [!WARNING]
> Some container resource metrics or latency metrics are marked as `unavailable`. This can occur due to host namespace isolation restrictions within the execution containers (e.g. lack of access to host cgroup parameters). Ensure appropriate permissions are enabled if absolute tracking is required.

