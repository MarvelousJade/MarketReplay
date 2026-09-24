# MarketReplay

**C++20 order-book reconstruction with multiple threads.** Reads market events, tracks live orders, and prints a deterministic final snapshot. Built to learn and discuss in C++ interviews—not a production trading system.

## Run it (Linux / WSL)

Needs GCC 11+, CMake 3.21+, Ninja, Python 3.10+.

```sh
cmake --preset release
cmake --build --preset release --parallel
ctest --preset release
./build/release/replay data/example.csv 2 64
```

Arguments: `input_file workers queue_capacity`. Snapshot → stdout; timing JSON → stderr.

## The whole design

```text
CSV → parser → bounded queues → worker-owned books → sorted snapshot
```

- **One reader** parses records and routes each symbol to a worker.
- **One worker per queue** updates its own books. No shared book locks.
- **Full queue?** The reader waits: backpressure, not dropped events.
- **Missing sequence?** Mark that book untrusted until a reset arrives.
- **Same input?** Same snapshot, regardless of worker count.

An order book stores outstanding buy/sell orders and total quantity at each price. This project reconstructs a feed; it does **not** match buyers with sellers.

## Learn it quickly

1. [Interview guide](docs/INTERVIEW.md): explanation, code reading order, common questions.
2. [Input and recovery rules](docs/ARCHITECTURE.md): the exact contract.
3. [Benchmarks](docs/BENCHMARKS.md): commands, measurements, limitations.

## Check correctness / performance

```sh
python3 python/reference.py data/example.csv > expected.csv
./build/release/replay data/example.csv > actual.csv 2> metrics.json
diff -u expected.csv actual.csv
python3 python/benchmark.py build/release/replay --events 100000 --repeats 3
```

Tests cover book updates, queue shutdown/backpressure, malformed input, and Python comparisons across worker counts. Release and ASan/UBSan passed locally. Full TSan validation is blocked by this WSL host's sanitizer startup failures; see the benchmark notes.

**Limits:** in-memory state, no checkpoint or network feed; queues are bounded but order storage is not. Restart by replaying the original file. More threads are not automatically faster.
