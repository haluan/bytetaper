# Request Coalescing Policy

Request Coalescing in ByteTaper allows you to deduplicate concurrent, identical GET requests to your upstream services. Instead of sending multiple requests for the same resource simultaneously, ByteTaper "parks" duplicate requests and satisfies them with a single upstream response, significantly reducing upstream load during traffic bursts.

## Configuration

Request coalescing is configured per-route in the ByteTaper policy YAML.

```yaml
routes:
  - id: "example-cached-route"
    match:
      prefix: "/api/v1/cached/"
    cache:
      enabled: true
      ttl_seconds: 300
    coalescing:
      enabled: true               # Enable coalescing for this route
      wait_window_ms: 50         # Max time (ms) followers wait for leader
      max_waiters_per_key: 64    # Max concurrent followers per unique request
      allow_authenticated: false  # Whether to allow coalescing for requests with Authorization
```

### Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | boolean | `false` | Enables request coalescing for the route. |
| `wait_window_ms` | integer | `25` | The maximum duration a follower request will wait for the leader's response before falling back to its own upstream call. |
| `max_waiters_per_key` | integer | `64` | The maximum number of concurrent requests that can be parked for a single unique request key. If exceeded, new requests will bypass coalescing. |
| `require_cache_enabled` | boolean | `true` | Coalescing requires a cache to be enabled on the route to distribute the response safely. |
| `allow_authenticated` | boolean | `false` | If `true`, requests with an `Authorization` header will be coalesced. Use with extreme caution as this can leak private data if the upstream does not vary responses by token. |

## How It Works

### Leader/Follower Model

1. **Leader**: The first request for a unique resource (defined by URI and headers) becomes the **Leader**. ByteTaper registers this request in an internal in-flight registry and forwards it to the upstream.
2. **Follower**: Subsequent concurrent requests for the same resource become **Followers**. ByteTaper "parks" these requests at the Envoy edge, holding the gRPC stream open without forwarding the request upstream.
3. **Completion**: When the Leader's response returns from the upstream, it is stored in the cache. ByteTaper then signals all parked Followers to resume. They perform a cache lookup, find the Leader's response, and return it to their respective clients.

### Safety & Constraints

- **HTTP Method**: Coalescing is strictly limited to **GET** requests. Mutations (POST, PUT, DELETE, etc.) are never coalesced to avoid side-effect interference.
- **Authentication**: By default, requests with an `Authorization` header bypass coalescing (`allow_authenticated: false`). This prevents the risk of sharing sensitive or user-specific data across different identities.
- **Wait Window Fallback**: If the Leader request takes longer than `wait_window_ms`, all parked Followers will automatically "fallback." They will stop waiting and proceed to make their own independent upstream calls. This ensures that a single slow upstream request doesn't stall all concurrent traffic indefinitely.

## Metrics

ByteTaper exports Prometheus metrics to monitor the effectiveness and health of the coalescing pipeline:

| Metric | Type | Description |
|--------|------|-------------|
| `bytetaper_coalescing_leader_total` | Counter | Total number of requests that acted as leaders. |
| `bytetaper_coalescing_follower_total` | Counter | Total number of requests parked as followers. |
| `bytetaper_coalescing_fallback_total` | Counter | Total number of followers that timed out and fell back to upstream. |
| `bytetaper_coalescing_max_waiters_exceeded_total` | Counter | Number of requests that bypassed coalescing because the waiter limit was reached. |

## API Intelligence

ByteTaper records "Coalescing Opportunities" when it detects bursts of duplicate GET requests on routes where coalescing is disabled. This data can be used by ByteTaper's intelligence layer to recommend enabling coalescing for specific high-traffic endpoints.
