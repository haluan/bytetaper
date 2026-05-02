# ByteTaper Observability

ByteTaper provides comprehensive observability through Prometheus metrics and HTTP response headers. These tools allow you to monitor the performance, savings, and safety status of your JSON optimizations.

## HTTP Response Headers

ByteTaper injects headers into the response to provide per-request diagnostics and waste reporting.

### Optimization Diagnostics
| Header Name | Value Type | Description |
| :--- | :--- | :--- |
| `x-bytetaper-transform-applied` | `true` \| `false` | Indicates if any mutation was performed on the body. |
| `x-bytetaper-extproc-response-body` | `true` | Indicates the ByteTaper External Processor handled the response body. |
| `x-bytetaper-route-policy` | `string` \| `none` | The ID of the policy route matched for this request. |

### Payload Metrics
| Header Name | Value Type | Description |
| :--- | :--- | :--- |
| `x-bytetaper-original-bytes` | `integer` | The size of the original payload before optimization. |
| `x-bytetaper-optimized-bytes` | `integer` | The size of the final payload sent to the client. |
| `x-bytetaper-waste-saved-bytes` | `integer` | Total bytes removed in this specific request. |
| `x-bytetaper-waste-removed-fields` | `integer` | Total number of JSON fields removed in this specific request. |

### Safety & Debugging
| Header Name | Value Type | Description |
| :--- | :--- | :--- |
| `x-bytetaper-fail-open-reason` | `string` | Emitted when transformation is skipped due to a safety constraint in fail-open mode. |
| `x-bytetaper-fail-closed-reason` | `string` | Emitted in the `ImmediateResponse` headers when a request is rejected in fail-closed mode. |

---

## Prometheus Metrics

The ByteTaper gRPC server exposes a `/metrics` endpoint with the following metrics:

### Core Counters
| Metric Name | Type | Description |
| :--- | :--- | :--- |
| `bytetaper_streams_total` | Counter | Total number of gRPC processing streams handled. |
| `bytetaper_bytes_original_total` | Counter | Total volume of original response bytes received from backends. |
| `bytetaper_bytes_optimized_total` | Counter | Total volume of bytes sent to clients after optimization. |
| `bytetaper_bytes_saved_total` | Counter | Total cumulative bytes saved (clamped at 0 per request to prevent negative reporting). |
| `bytetaper_fields_removed_total` | Counter | Total number of JSON fields removed across all processed requests. |
| `bytetaper_transform_applied_total` | Counter | Count of requests where a transformation was actually applied. |
| `bytetaper_transform_errors_total` | Counter | Count of requests that encountered processing errors during transformation. |

### Savings & Efficiency
| Metric Name | Type | Description |
| :--- | :--- | :--- |
| `bytetaper_payload_reduction_ratio` | Gauge | The overall efficiency ratio: `(original - optimized) / original`. |
| `bytetaper_route_waste_saved_bytes_total` | Counter | Bytes saved, partitioned by `route` label (e.g., `{route="api-v1-users"}`). |

---

#### Common Fail-Open Reasons:
*   `skip_unsupported`: Content-type was not JSON, or the structure was too complex for current logic.
*   `invalid_json_safe_error`: The backend returned malformed JSON.
*   `output_too_small`: The optimized result would have exceeded the pre-allocated safety buffer.
*   `invalid_input`: Empty or null input received from the server.
