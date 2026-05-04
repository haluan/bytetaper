# ByteTaper Benchmarks

This directory contains scripts and configuration for reproducible performance benchmarking of ByteTaper.

## Usage

Benchmarks are executed inside a dedicated Docker container to ensure consistent results across different environments. No local benchmarking tools are required.

### Run Default Benchmark

```bash
docker compose run --rm bytetaper-benchmark
```

This command will:
1. Start the core ByteTaper services (Envoy, ExtProc, Mock API).
2. Execute `wrk2` against the configured target.
3. Save the results to `reports/benchmarks/`.

### Custom Scenarios

You can run custom benchmark scripts or pass arguments to the benchmark container:

```bash
docker compose run --rm bytetaper-benchmark ./benchmarks/scenarios/my_custom_test.sh
```

## Tools

- **wrk2**: Used for constant throughput benchmarking. It mitigates the "coordinated omission" problem present in many other tools, providing more accurate latency percentiles.

## Reports

Benchmark results are stored in the `reports/benchmarks/` directory as timestamped text files. These reports include:
- Throughput (Requests per second)
- Latency distribution (Percentiles: 50%, 75%, 90%, 99%, 99.9%, etc.)
- Transfer rate
- HTTP error counts (if any)
