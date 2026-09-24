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

## Local smoke result (not a performance guarantee)

100,000 events, seed 42, capacity 1024, three measured runs; GCC 14.2.1 Release, shared WSL2 host, no explicit CPU pinning:

| Workers | Median events/sec | Median run p99 upper bound |
|---|---:|---:|
| 1 | 831,859 | 262 µs |
| 2 | 329,832 | 66 µs |
| 4 | 72,971 | 66 µs |

**Takeaway:** more threads were slower here. Lock handoff, scheduling, and a single dispatcher are hypotheses—not measured causes. The lower queue latency does not imply higher throughput. Full local metadata remains in the ignored benchmark report.

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
