# Compression Policy

The Compression Policy enables ByteTaper to coordinate response compression with downstream Envoy filters. Instead of performing resource-intensive compression itself, ByteTaper evaluates eligibility and signals the Envoy native compressor via specific headers.

## Configuration Schema

Compression is configured per-route within the `compression` block:

```yaml
compression:
  enabled: true
  min_size_bytes: 1024          # Minimum threshold (1KB)
  eligible_content_types:       # List of MIME types to compress
    - application/json
  preferred_algorithms:         # Algorithm priority
    - gzip
    - br
    - zstd
  already_encoded_behavior: skip # Behavior if response is already encoded
```

### Fields

| Field | Type | Description | Default |
| :--- | :--- | :--- | :--- |
| `enabled` | boolean | Enables compression coordination for this route. | `false` |
| `min_size_bytes` | integer | Threshold in bytes. Responses smaller than this will not be signaled for compression. | `0` (none) |
| `eligible_content_types` | string[] | List of `Content-Type` values that are eligible for compression. Max 8 types. | `[]` |
| `preferred_algorithms` | string[] | Ordered list of supported algorithms (`gzip`, `br`, `zstd`). | `[]` |
| `already_encoded_behavior`| string | `skip`: Do not signal if already encoded. <br> `passthrough`: Metadata tracking only. | `skip` |

## Eligibility Logic

ByteTaper evaluates compression eligibility based on three primary factors:

1.  **Client Support**: Parses the `Accept-Encoding` request header to ensure the client supports at least one of the preferred algorithms.
2.  **Payload Type**: Compares the upstream `Content-Type` header against the `eligible_content_types` list.
3.  **Payload Size**: Compares the upstream `Content-Length` header (if available) against `min_size_bytes`.

## Coordination Mechanism

If a response is deemed eligible, ByteTaper injects the following headers into the gRPC `ext_proc` response:

*   `x-bytetaper-compression-candidate: true`
*   `x-bytetaper-compression-algorithm-hint: <algorithm>` (e.g., `gzip`)

Envoy must be configured with a compressor filter that triggers on the `x-bytetaper-compression-candidate` header.

## API Intelligence & Metrics

ByteTaper records route-level compression opportunities:

*   **Signals**:
    *   `large_uncompressed_response_seen`: High-waste potential.
    *   `client_supports_compression_seen`: Potential for bandwidth savings.
    *   `compression_candidate_seen`: Successful ByteTaper coordination.
    *   `compression_skipped_too_small_seen`: Efficient skip decision.
*   **Recommendation Codes**:
    *   `large_uncompressed_json_response_seen`: High-confidence recommendation for enabling compression.

## Implementation History

This documentation captures the state of the compression module as finalized in the following commit range (`c24883f` to `7e65368`):

- **7e65368**: Record compression opportunity for API Intelligence
- **bd8096d**: Integration test with Envoy compression filter
- **b10011f**: Compression safety validation
- **091c4da**: Add compression metrics
- **cb75e6f**: Add compression candidate header
- **8d6f845**: Add compression candidate decision
- **69bd057**: Add minimum size eligibility
- **acb08ea**: Add compression Content-Type eligibility
- **b1d9d23**: Detect response Content-Encoding
- **37066e6**: Parse Accept-Encoding header
