# Envoy API Proto Snapshot

This repository vendors a minimal Envoy API proto snapshot for ByteTaper ext_proc adapter bootstrap.

- Upstream repository: https://github.com/envoyproxy/envoy
- Pinned tag: v1.33.0
- Source root used for snapshot: `api/`
- Seed file: `envoy/service/ext_proc/v3/external_processor.proto`

Notes:
- Files under `proto/` are source snapshots required to compile protobuf message types.
- Generated `.pb.h/.pb.cc` are build artifacts and are not committed.
- `udpa/*`, `xds/*`, and `validate/*` files are local compatibility subsets used only
  to satisfy option/type imports during protobuf C++ generation in this phase.
