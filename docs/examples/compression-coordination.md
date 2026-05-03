# Compression Coordination with ByteTaper and Envoy

ByteTaper does not compress response bodies. Envoy owns compression execution.
ByteTaper's role is to evaluate compression eligibility and emit diagnostic headers.

## Architecture

```
Client Request (Accept-Encoding: br, gzip)
        │
        ▼
Envoy ExtProc filter  ──►  ByteTaper
        │                    │
        │                    │  Evaluates:
        │                    │  - policy.compression.enabled
        │                    │  - client Accept-Encoding
        │                    │  - response Content-Type eligible
        │                    │  - response size >= min_size_bytes
        │                    │  - not already encoded
        │                    │
        │◄────────────────────
        │  Emits headers:
        │  x-bytetaper-compression-candidate: true
        │  x-bytetaper-compression-reason: (none / policy_disabled / ...)
        │  x-bytetaper-compression-algorithm-hint: br
        │
        ▼
Envoy compressor filter
        │  Reads: Accept-Encoding from original request
        │  Applies: gzip (or brotli with brotli library)
        │  Sets: Content-Encoding: gzip
        ▼
Client receives compressed response
```

## What ByteTaper Does

- Reads `Accept-Encoding` from the client request
- Reads `Content-Type` and `Content-Encoding` from the upstream response
- Checks the route's `compression` policy (enabled, min_size_bytes, eligible_content_types)
- Emits `x-bytetaper-compression-candidate` as a diagnostic/observability header

## What ByteTaper Does NOT Do

- ByteTaper does **not** compress response bodies
- ByteTaper does **not** set `Content-Encoding` on responses
- ByteTaper does **not** require any compression library (gzip, brotli, zstd)
- ByteTaper does **not** intercept or modify the `Accept-Encoding` negotiation

## Example Policy (bytetaper-policy.yaml)

```yaml
- route_id: api-v1-compressed
  match_kind: prefix
  match_prefix: /api/v1/
  mutation: full
  compression:
    enabled: true
    min_size_bytes: 1024
    eligible_content_types:
      - application/json
      - text/plain
    preferred_algorithms:
      - brotli
      - gzip
    already_encoded_behavior: skip
```

## Start with the compression-enabled Envoy config
docker compose run --rm envoy \
  -c /etc/envoy/envoy-compression.yaml
```

## Diagnostic Headers

| Header | Values | Meaning |
|--------|--------|---------|
| `x-bytetaper-compression-candidate` | `true` / `false` | Whether ByteTaper marked this response eligible |
| `x-bytetaper-compression-reason` | `policy_disabled`, `no_client_support`, `content_type_not_eligible`, `below_minimum`, etc. | Why the response was not a candidate (absent when candidate=true) |
| `x-bytetaper-compression-algorithm-hint` | `gzip`, `br`, `zstd` | Algorithm ByteTaper would prefer (Envoy decides the actual algorithm) |
