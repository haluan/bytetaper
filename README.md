# ByteTaper: API response optimization without backend rewrites

## Development Commands

Use Docker Compose workflows only. Do not run local host build/test commands.

Build/demo flow:

```bash
docker compose run --rm bytetaper-build
```

Unit-test flow:

```bash
docker compose run --rm bytetaper-unit-test
make test
```

Smoke-test flow:

```bash
docker compose run --rm bytetaper-smoke-test
make smoke-test
```

Integration-test flow:

```bash
docker compose run --rm bytetaper-integration-test
```

Format flow:

```bash
make format
```

Compatibility note:
If `docker compose` is unavailable in your environment, use the equivalent
`docker-compose` command form.

Rootless note:
Containers run with host-mapped UID/GID to avoid root-owned files in mounted
workspaces. Example:

```bash
LOCAL_UID=$(id -u) LOCAL_GID=$(id -g) docker compose run --rm bytetaper-unit-test
```

## APG Stage Behavior

Stage behavior contract:
- Stage input is mutable `ApgTransformContext&`.
- Stage output is `StageOutput{ result, note }`.
- `StageResult::Continue` executes the next stage.
- `StageResult::Error` stops the pipeline immediately and returns that exact output.
- `StageResult::SkipRemaining` stops the pipeline immediately (non-error flow) and
  returns that exact output.
- `note` is non-owning (`const char*`) and is propagated unchanged.

Pipeline trace behavior:
- Trace fields live in context: `trace_buffer`, `trace_capacity`, `trace_length`.
- Trace is reset at pipeline start.
- One token is appended per executed stage:
  `C` (Continue), `E` (Error), `S` (SkipRemaining).
- Trace writes are skipped when buffer is null or capacity is zero.
- Trace writes are safely truncated and never write out-of-bounds.
