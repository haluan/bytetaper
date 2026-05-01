# Field Selection

Field Selection is a high-performance optimization primitive that reduces API payload size by selectively keeping or removing JSON response fields at the edge.

## Description

ByteTaper's field selection engine allows operators to define rules that strip unnecessary data from backend responses. By reducing the number of fields sent to the client, ByteTaper minimizes bandwidth usage, lowers egress costs, and reduces client-side parsing overhead. The engine is implemented in Orthodox C++ with zero heap allocations during the matching process, ensuring minimal latency impact.

## How to Setup

Field selection is configured per route within the `field_filter` block of your YAML policy file.

1.  **Define a Route**: Ensure your route has a unique `id` and a `match` pattern.
2.  **Add `field_filter`**:
    -   Set `mode` to either `allowlist` (to keep only specific fields) or `denylist` (to remove specific fields).
    -   Add a list of `fields` to the policy.

Example configuration:

```yaml
routes:
  - id: "user-profile-optimized"
    match:
      kind: "prefix"
      prefix: "/api/v1/users"
    field_filter:
      mode: "allowlist"
      fields:
        - "id"
        - "username"
        - "avatar_url"
```

## Behavior Reference

| Example Scenario | Policy Configuration | Expected Result |
| :--- | :--- | :--- |
| **Keep specific fields** | `mode: allowlist`, `fields: ["id", "name"]` | Only `id` and `name` are kept; all other top-level fields are stripped. |
| **Strip sensitive data** | `mode: denylist`, `fields: ["internal_id", "hash"]` | `internal_id` and `hash` are removed; all other fields are kept. |
| **Nested Objects** | `mode: allowlist`, `fields: ["user.id", "user.name"]` | Keeps only `id` and `name` inside the `user` object. |
| **Arrays of Objects** | `mode: allowlist`, `fields: ["items[].price"]` | For each object in `items`, keeps only the `price` field. |
| **Empty Allowlist** | `mode: allowlist`, `fields: []` | All fields are stripped (resulting in an empty JSON object). |
| **Empty Denylist** | `mode: denylist`, `fields: []` | All fields are kept (no fields match the removal criteria). |
| **Pass-through (default)** | `mode: none` (or block absent) | The response body is passed through unchanged. |

## Advanced Implementation Details

ByteTaper's field selection engine has been upgraded from simple shallow matching to a robust, streaming nested filter.

### Nested Path Support

You can now target specific data within complex JSON structures using two types of notation:
-   **Dotted Notation (`a.b.c`)**: Target fields within nested objects.
-   **Array Notation (`items[]`)**: Target fields within every object contained in an array.

### Streaming Performance

The engine uses a custom "Flat JSON" parser designed for extreme efficiency:
-   **Zero Heap Allocation**: No temporary objects or strings are created during parsing.
-   **Single Pass**: Data is filtered and written to the output buffer in a single streaming pass.
-   **Bounded Memory**: The parser uses fixed-size stack buffers (e.g., 512 bytes for path tracking) to prevent memory exhaustion attacks.

### Metrics & Visibility

ByteTaper tracks the efficiency of your field selection policies in real-time:
-   **Encountered Fields**: Total number of fields found in the original response.
-   **Emitted Fields**: Total number of fields kept in the optimized response.
-   **Removed Field Count**: Automatically reported in the transformation context for observability.

## Constraints & Limits

-   **Max Selection Path**: Path strings are limited to 512 characters.
-   **Path Depth**: Deeply nested structures are supported as long as the full path fits within the 512-character limit.
-   **Scalar Array Filtering**: Array notation (`[]`) is optimized for arrays of objects. Filtering specific indices (e.g., `items[0]`) is currently not supported.
-   **Invalid JSON**: If a response contains invalid JSON, ByteTaper safely aborts the transformation and returns an error response (`{"error":"invalid_json"}`) to prevent data corruption.
