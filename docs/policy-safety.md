# ByteTaper Policy Safety

This document outlines the safety mechanisms and architectural guardrails implemented in ByteTaper to ensure system resilience, data integrity, and deterministic behavior when processing high-volume traffic.

## Architectural Safety Principles

ByteTaper is designed with a "Safety First" mindset. The following mechanisms protect the system from misconfigurations, resource exhaustion, and upstream anomalies.

### 1. Fail-Closed vs. Fail-Open Modes
ByteTaper supports two fundamental failure modes:
- **Fail-Open (Default)**: If an error occurs (e.g., malformed JSON, timeout), ByteTaper passes the original response to the client untouched, ensuring availability over transformation.
- **Fail-Closed**: When strict compliance is required, ByteTaper can be configured to reject the response if a transformation fails, ensuring that no unfiltered data ever reaches the client.

### 2. Resource Guardrails
- **Max Body Size**: To prevent memory exhaustion (OOM), ByteTaper enforces a `max_response_bytes` limit. Responses exceeding this limit are bypassed or rejected depending on the configuration.
- **L1/L2 Cache Boundaries**: Fixed-size L1 buffers ensure deterministic memory usage, while persistent L2 storage prevents disk-space runaway.

### 3. Transformation Safety
ByteTaper automatically skips transformation in the following scenarios to ensure original response integrity:
- **Non-2xx Responses**: Error responses from upstreams (e.g., 404, 500) are bypassed to preserve the original error message and structure.
- **Non-JSON Content**: Only `application/json` responses are processed. Other content types (e.g., binary, HTML) are passed through without mutation.
- **Policy Not Found**: If no route policy matches the request path, ByteTaper falls back to a safe "no-op" state.

### 4. Deterministic Execution
- **Timeout Protection**: All processing stages are governed by deadlines. If a transformation takes too long (e.g., complex regex or large JSON), the operation times out and falls back to the configured safety mode.
- **Malformed Input Resilience**: The JSON parser is hardened against "billion laughs" attacks and corrupt payloads, ensuring a crash-safe environment.

## Policy Validation

ByteTaper enforces safety at the configuration layer:
- **Schema Validation**: All policy YAML files are validated against a strict schema during load time. Invalid policies (e.g., missing mandatory fields or invalid regex patterns) cause the server to reject the configuration.
- **Pre-flight Checks**: Tools are provided to validate policies before deployment to ensure that matchers and mutations are semantically correct.

## Observe Mode (Non-Mutating)

For risk-free deployment in production environments, ByteTaper supports **Observe Mode** (`mutation: none` or `mutation: disabled`).
- In this mode, ByteTaper performs all lookups and filtering logic internally but **does not modify the response body**.
- It injects a report header (`x-bytetaper-fail-open-reason: observe_mode`) allowing teams to analyze the impact of policies before enabling active mutation.

## Summary of Safety Reasons

When a bypass occurs, ByteTaper injects the `x-bytetaper-fail-open-reason` header with one of the following codes:

| Reason Code | Description |
| :--- | :--- |
| `body_too_large` | Response body exceeded the `max_response_bytes` limit. |
| `non_2xx_response` | Upstream returned a non-success status code. |
| `non_json_response` | Response `Content-Type` was not `application/json`. |
| `policy_not_found` | No matching route policy was found for the request. |
| `invalid_policy` | The matched policy was semantically invalid or corrupt. |
| `timeout` | Processing exceeded the allocated deadline. |
| `observe_mode` | Policy is configured to report only without mutating data. |
| `malformed_json` | The upstream response body could not be parsed. |
| `unknown_error` | An internal error occurred during processing. |
