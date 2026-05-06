-- SPDX-FileCopyrightText: 2026 Haluan Irsad
-- SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

function done(summary, latency, requests)
   local p50 = latency:percentile(50.0) / 1000.0
   local p95 = latency:percentile(95.0) / 1000.0
   local p99 = latency:percentile(99.0) / 1000.0
   
   print(string.format("LATENCY_PERCENTILES: p50=%.3f ms, p95=%.3f ms, p99=%.3f ms", p50, p95, p99))
end
