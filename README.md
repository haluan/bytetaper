# ByteTaper: API response optimization without backend rewrites

## Development (Docker Compose only)

Run the development build container from the repository root.
For this phase, the command configures CMake into `./build`, builds
`bytetaper-extproc`, runs a CTest smoke test, and executes the binary:

```bash
docker compose run --rm bytetaper-build
```

Local host build/test workflows are out of scope. Use Docker Compose commands only.

Format C++ sources:

```bash
make format
```

Run test flow in Docker Compose:

```bash
make test
```

Canonical unit-test service command:

```bash
docker compose run --rm bytetaper-unit-test
```
