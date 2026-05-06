# JSON Filtering & Parser Performance Investigation Report

This report presents a thorough performance investigation of ByteTaper's JSON parsing and filtering hot paths. It analyzes microbenchmark results, identifies parser bottlenecks, classifies branches, and provides an explicit decision-oriented recommendation for future optimization.

---

## 1. Benchmarking Methodology & Environment

### Methodology
To measure hot-path performance without adding external dependencies, we implemented an in-process microbenchmark harness in [benchmarks/json_filter_benchmark.cpp](benchmarks/json_filter_benchmark.cpp).
- **Warmup Cycles**: 1,000 warmup iterations to ensure instruction cache lines and CPU pipelines are warm.
- **Measured Cycles**: 50,000 iterations measured using `std::chrono::high_resolution_clock`.
- **Control Slices**: For each scenario, we run a "no-op selection" control alongside standard selection filtering to cleanly isolate the baseline parser traversal cost from the output-copy/construction overhead.
- **Fixture Tiers**: Sized in Small, Medium, and Large tiers for:
  - **Flat Object**: Simple flat JSON structures.
  - **Nested Object**: Multi-level nested structures requiring recursive stack-based parsing.
  - **Object Array**: JSON documents with arrays of objects requiring wildcard wildcard wildcard (`items[].qty`) path evaluation.

### Execution Environment
Benchmarks are compiled under C++20 with full optimizations (`-O3`) and run inside a containerized Debian dev environment via Docker Compose to guarantee environment isolation.

---

## 2. Microbenchmark Results

The table below summarizes the exact execution timings (in nanoseconds per operation) and the output buffer sizes produced:

| Scenario | Fixture Size | Selection | Ns/Op | Output Size (Bytes) | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Flat** | Small | Standard | **311.6 ns** | 39 | Ok |
| **Flat** | Small | Control | 136.9 ns | 2 | Ok |
| **Flat** | Medium | Standard | **534.3 ns** | 67 | Ok |
| **Flat** | Medium | Control | 170.0 ns | 2 | Ok |
| **Flat** | Large | Standard | **596.9 ns** | 67 | Ok |
| **Flat** | Large | Control | 151.1 ns | 2 | Ok |
| **Nested** | Small | Standard | **700.8 ns** | 34 | Ok |
| **Nested** | Small | Control | 334.5 ns | 2 | Ok |
| **Nested** | Medium | Standard | **3453.3 ns** | 74 | Ok |
| **Nested** | Medium | Control | 852.9 ns | 2 | Ok |
| **Nested** | Large | Standard | **2654.0 ns** | 74 | Ok |
| **Nested** | Large | Control | 1652.0 ns | 2 | Ok |
| **Array** | Small | Standard | **796.9 ns** | 33 | Ok |
| **Array** | Small | Control | 359.2 ns | 2 | Ok |
| **Array** | Medium | Standard | **1525.1 ns** | 59 | Ok |
| **Array** | Medium | Control | 661.9 ns | 2 | Ok |
| **Array** | Large | Standard | **5236.1 ns** | 192 | Ok |
| **Array** | Large | Control | 1639.8 ns | 2 | Ok |

### Key Findings & Observations
1. **Flat Object Parser is Blazing Fast**:
   Flat object traversal and parsing completes in **under 600 ns** even for large objects containing up to 20 keys. This is because flat parsing utilizes a single-pass indexer to find offsets and caches them directly in a fixed-size `ParsedFlatJsonObject` table with $O(1)$ lookup complexity.
2. **Path Construction is the Primary Overhead**:
   Recursive parsing of nested objects or object arrays is **3x to 5x slower** than flat parsing. The control timings demonstrate that traversing a medium nested object takes `852.9 ns` (compared to `170.0 ns` for a flat object of equivalent complexity). This difference is largely driven by string-based path building (`build_full_path`) and string comparison matching for every nested key.
3. **Array Wildcard Matching Overhead Scale linearly**:
   Filtering a large object array with wildcard matching (`items[].qty`) takes **5.2 microseconds** for 8 elements. The overhead scales with the number of array elements because the path matcher must validate wildcards on every item.

---

## 3. Hotspot Analysis of Parser and Matchers

A detailed code review of `src/json_transform/flat_json.cpp` and `src/json_transform/filter_flat_json.cpp` highlights three critical hotspots:

1. **Path Construction (`build_full_path`)**:
   In `src/json_transform/filter_flat_json.cpp`, nested path evaluation formats keys into stack-allocated character arrays using `std::strncpy` / character appending to produce strings like `"user.profile.email"`. For deep JSON structures, this dynamic string copying on the stack adds significant instruction overhead.
2. **Linear Path Matching (`match_selection_compiled`)**:
   Precompiled route matching compares the constructed stack path against all active selections linearly. Since `policy::kMaxFields` is limited to 16, this linear scan is relatively small, but is repeated for every field key found in the document.
3. **Character-by-character Whitespace Skipping**:
   The `skip_whitespace` routine inspects chars one by one. While robust, skipping long stretches of whitespace or pretty-printed JSON results in high instruction counts compared to vectorized (SIMD) skips.

---

## 4. Branch Analysis & Classification

JSON parser performance is dominated by CPU branch predictor accuracy. We classify these branches into two categories:

### A. Structural Correctness Branches (Essential - DO NOT REWRITE)
These branches are required to guarantee syntactic validation, parse boundaries, and JSON standards compliance:
- **String Token Recognition (`ch == '"'`) & Escape Sequences (`ch == '\\'`)**: Necessary to skip string content correctly and prevent arbitrary payload injection.
- **Whitespace skipping loop**: Essential to align indexes with token boundaries.
- **Literal character validations**: Matching `true`, `false`, `null` keyword values accurately.
- **Array / Object Boundary Checks**: Essential to match braces (`{`, `}`, `[`, `]`) and handle depths safely.
- *Recommendation*: **Do not remove or change these branches.** They are critical to safety, robust bounds verification, and preventing security exploits (e.g. invalid JSON parser bypasses).

### B. Candidate Optimization Branches (Actionable Overheads)
These branches are driven by internal implementation choices and represent potential optimization vectors:
- **Full Path Copying**: Rebuilding prefix strings during recursion can be completely replaced by a non-copying path pointer context list (e.g., matching using a chain of string views on the stack).
- **Wildcard String-Scan Checks**: The parser checks `selection_contains_unsupported_array_notation` via a loop on every run. Precompiling this check during policy loading would save execution cycles on the hot path.

---

## 5. Explicit Recommendation

> [!IMPORTANT]
> Based on the gathered performance metrics and structural analysis, we make the following explicit recommendation:
>
> **RETAIN CURRENT PARSER & INITIATE MINOR TARGETED OPTIMIZATIONS**
>
> **Rationale**:
> - Flat object parsing and filtering executes in **under 600 ns** which is extremely optimized and satisfies hot-path latency targets.
> - A complete parser rewrite (or introducing heavy third-party JSON parser libraries) is **not justified** and would introduce massive code complexity, heap allocations, and binary footprint overhead, which directly violates our Orthodox C++ philosophy.
> - Future work should focus exclusively on two targeted optimizations for nested and array structures:
>   1. **Zero-Copy Path Representation**: Replace stack-string concatenation in nested objects with a stack-allocated link-node tree to match fields without string copying.
>   2. **Precompiled Wildcard Checks**: Avoid scanning selections for array wildcard notation on every request by caching this state in the `CompiledRouteRuntime` table.
