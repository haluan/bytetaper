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
