# Tests and performance

## Sanitizers

```sh
cmake --preset asan && cmake --build --preset asan --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --preset asan

cmake --preset tsan && cmake --build --preset tsan --parallel
TSAN_OPTIONS=halt_on_error=1 ctest --preset tsan
```

- **ASan:** invalid memory access/leaks. **UBSan:** undefined behavior.
- **TSan:** data races; run separately from ASan.
- Local GCC 14.2.1 / WSL2: Release and ASan/UBSan passed. TSan unit tests passed once, but other runs failed during startup (`unexpected memory mapping` / exit -11). Full TSan validation is still needed on compatible Linux.
- `.github/workflows/ci.yml` runs these builds when MarketReplay is a standalone GitHub repository. Nested workflows do not run in the parent repository.

## Reproduce a benchmark

```sh
python3 python/benchmark.py build/release/replay --events 100000 --repeats 3
```

Fixed seed, 32 symbols, mixed adds/cancels/executions, at most 128 live orders per symbol. One warmup per worker count. All snapshots must have the same hash.

`bench-results/report.json` saves raw runs, CPU/affinity, build settings, input/binary hashes, and commands. Use longer runs (e.g. one million events, five repeats) for serious comparisons. Keep hardware, compiler, seed, and CPU affinity fixed; avoid background load.

### What the numbers mean

- **Throughput:** parsed events / replay seconds. Includes parsing, handoff, updates, thread launch and drain; excludes final output.
- **Latency:** immediately before queue push → after book update. Includes backpressure and queue wait; excludes parsing. Not exchange-to-trade latency.
- **p99:** 99% of samples fit below this reported bucket bound. Histograms use power-of-two buckets, so these are approximate upper bounds, not exact timings.
- **Queue high-water:** maximum observed queue size. **Blocked pushes:** pushes that encountered a full queue, not time spent blocked.

Every event is timed. This is an unpaced replay benchmark, not a realistic external-arrival latency test. Do not benchmark sanitizer builds.

## Original local smoke result (historical, not a performance guarantee)

100,000 events, seed 42, capacity 1024, three measured runs; GCC 14.2.1 Release, shared WSL2 host, no explicit CPU pinning:

| Workers | Median events/sec | Median run p99 upper bound |
|---|---:|---:|
| 1 | 831,859 | 262 µs |
| 2 | 329,832 | 66 µs |
| 4 | 72,971 | 66 µs |

**Takeaway:** more threads were slower here. Lock handoff, scheduling, and a single dispatcher are hypotheses—not measured causes. The lower queue latency does not imply higher throughput. Full local metadata remains in the ignored benchmark report.

## Rework local smoke result

Fresh Release run at `a944c2e`: GCC 14.2.1, Python 3.14.7, WSL2 Linux 6.18.40.1, Ryzen 5 3600, 12 visible logical CPUs (affinity 0–11, no explicit pinning). Same 100,000-event, seed-42, 32-symbol, capacity-1024 workload; one warmup and three measurements per configuration.

```sh
python3 python/benchmark.py build/release/replay --events 100000 --repeats 3 --output bench-results/rework
```

| Workers | Warmup events/s | Three measured events/s | Median events/s | Median run p99 upper ns |
|---|---:|---|---:|---:|
| 1 | 690,194 | 694,923 / 726,876 / 695,308 | 695,308 | 262,143 |
| 2 | 434,197 | 447,608 / 447,977 / 416,889 | 447,608 | 131,071 |
| 4 | 76,153 | 72,720.8 / 66,092 / 68,526.6 | 68,526.6 | 131,071 |

All 12 snapshots matched SHA-256 `7afa85002082e9e7f80d6506179981b5ffd778bc3fb524f06853c29aec4e2112`.
Input SHA-256: `9ee4339a90d738b57f0ecb777e4b1c42b6cdfb60a77cb4ad8f8399a269b0118e`.
Binary SHA-256: `c10c6a4a3d3ec2dba448a5c06fe33ed508bb2815f8bd2d10ce8e0362f114824a` (compiler/build dependent).

Raw local runs and detailed host/build metadata are retained in ignored `bench-results/rework/report.json`; the measured samples above are committed so the evidence does not depend on that local artifact. This short synthetic workload supports retaining a simple architecture, not claiming a speed improvement. The TSan build/check also ran concurrently during this smoke collection, adding uncontrolled load; these are not clean comparative performance measurements. No profiling, production load, or synchronous-path comparison was performed. Fresh release/ASan/TSan check outcomes are in [REWORK.md](REWORK.md).

## Profile before optimizing

```sh
cmake --preset profile && cmake --build --preset profile --parallel
python3 python/generate.py /tmp/events.csv --events 1000000
perf stat -r 5 -e cycles,instructions,cache-misses,context-switches \
  ./build/profile/replay /tmp/events.csv 2 1024 > /dev/null
perf record -g --call-graph dwarf -- \
  ./build/profile/replay /tmp/events.csv 2 1024 > /dev/null
perf report
```

Requires Linux `perf` and host permissions. Profiling was not run locally. Look for time in parsing, map allocation, queue locks, and scheduling; change one thing, rerun correctness tests, then compare repeated benchmarks.
