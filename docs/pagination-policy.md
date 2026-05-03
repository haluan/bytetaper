# Pagination Policy

The Pagination Policy controls how ByteTaper handles paginated requests to upstream APIs. It allows for automatic injection of default limits, enforcement of maximum limits, and risk assessment based on response sizes.

## Configuration Schema

Pagination is configured per-route within the `pagination` block:

```yaml
pagination:
  enabled: true
  mode: limit_offset            # Options: limit_offset, cursor
  limit_param: limit            # Name of the query parameter for limit
  offset_param: offset          # Name of the query parameter for offset
  default_limit: 20             # Applied if no limit is provided by the client
  max_limit: 100                # Capped if the client requests more
  upstream_supports_pagination: true
  max_response_bytes_warning: 524288 # Bytes (e.g., 512KB)
```

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
| `enabled` | boolean | Enables pagination handling for this route. |
| `mode` | string | `limit_offset`: Standard limit/offset query param mutation. <br> `cursor`: Cursor-based pagination (Metadata tracking only; mutation not yet implemented). |
| `limit_param` | string | The name of the query parameter used for the limit (e.g., `limit`, `count`, `per_page`). Max 31 characters. |
| `offset_param` | string | The name of the query parameter used for the offset (e.g., `offset`, `skip`, `start`). Max 31 characters. |
| `default_limit` | integer | The limit value to inject if the client omits it. |
| `max_limit` | integer | The maximum allowed limit. If a client requests more, ByteTaper will rewrite the request to this value. |
| `upstream_supports_pagination` | boolean | Informational flag for risk assessment. |
| `max_response_bytes_warning` | integer | Threshold in bytes. If a response exceeds this size, it is flagged as high risk for inefficient pagination. |

## Validation Rules

ByteTaper performs strict validation on the pagination block:

1.  **Mode Required**: If `enabled` is `true`, `mode` must be specified.
2.  **Implementation Status**: `cursor` mode is currently defined in the schema but will fail validation with `cursor_mode_not_implemented` if selected for mutation.
3.  **Parameter Length**: `limit_param` and `offset_param` must be shorter than 32 characters.
4.  **Safe/Production Validation**: When loading policies for production use, `max_limit` must be explicitly set to a value greater than `0`.

## Behavior

*   **Mutation**: If the route is in `mutation: full` mode, ByteTaper will reconstruct the request URL to inject the `default_limit` or cap the client's limit to `max_limit`.
*   **Observe Mode**: If the route is in `mutation: observe` mode, ByteTaper will identify limit violations and record metrics/risk signals without modifying the upstream request.
