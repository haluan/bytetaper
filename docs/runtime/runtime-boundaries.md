# ByteTaper Runtime Execution Boundaries

ByteTaper processes Envoy ExtProc gRPC streams on request threads. These threads
must not perform disk I/O, network I/O, or unbounded computation. Background work
runs through the `runtime::WorkerQueue`, which is limited to cache I/O only.

## Hot-Path Allowed (synchronous, in request/response pipeline)

| Operation | Stage |
|-----------|-------|
| Policy match | (pre-pipeline, grpc_server.cpp) |
| L1 cache lookup / store | `l1_cache_lookup_stage`, `l1_cache_store_stage` |
| Coalescing decision | `coalescing_decision_stage` |
| Coalescing follower wait — L1-only, bounded | `coalescing_follower_wait_stage` |
| Async L2 enqueue (non-blocking) | `l2_cache_async_lookup_enqueue_stage`, `l2_cache_async_store_enqueue_stage` |
| Pagination mutation | `pagination_request_mutation_stage` |
| Compression decision | `compression_decision_stage` |
| Field selection / JSON transform | (bounded by safety limits) |
| Metrics counter update | `record_*_event()` |
| Fail-open / fail-closed safety | `evaluate_filter_safety()` |

## Hot-Path Forbidden

The following must NOT appear in `kLookupStages` or `kStoreStages`:

| Forbidden Operation | Reason |
|---------------------|--------|
| `l2_cache_lookup_stage` | Synchronous RocksDB get — disk I/O |
| `l2_cache_store_stage` | Synchronous RocksDB put — disk I/O |
| Any disk cleanup / warmup stage | Unbounded latency |
| Any backend / network call | Request thread starvation |
| Unbounded body processing | Memory/CPU spike |

## Worker Queue Allowed (`RuntimeJobKind`)

| Job | Description |
|-----|-------------|
| `L2LookupJob` | RocksDB get + L1 promotion |
| `L2StoreJob` | RocksDB put |
| `L2RemoveJob` (future) | RocksDB key removal |
| `L2WarmupJob` (future) | Prefetch into L1 |
| `L2TtlCleanupJob` (future) | Expired-entry sweep |

## Worker Queue Forbidden

The worker queue must never execute:

- Backend HTTP calls or gRPC stream writes
- JSON transform, compression, or coalescing wait jobs
- Pagination mutation jobs
- General-purpose APG stage execution (`run_pipeline`)

## Enforcement

The arrays `kLookupStages` and `kStoreStages` in `include/extproc/default_pipelines.h`
are inspected by `tests/default_pipeline_boundaries_test.cpp`. Any new stage added to
these arrays that calls `cache::l2_get()` or `cache::l2_put()` synchronously violates
this contract.
