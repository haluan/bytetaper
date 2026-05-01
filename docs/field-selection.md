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
| **Pass-through (default)** | `mode: none` (or block absent) | The response body is passed through unchanged. |
| **Empty Allowlist** | `mode: allowlist`, `fields: []` | All fields are stripped (resulting in an empty JSON object). |
| **Empty Denylist** | `mode: denylist`, `fields: []` | All fields are kept (no fields match the removal criteria). |

> [!NOTE]
> **Shallow Matching**: The current implementation performs shallow matching on top-level JSON fields. Nested field filtering within child objects is reserved for future updates.
