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
4. [Rework audit](docs/REWORK.md) and [decisions](docs/DECISIONS.md): actual changes and evidence.
5. [Debugging practice](docs/DEBUGGING.md): separate learning checkpoints, without spoilers.

## Check correctness / performance

```sh
python3 python/reference.py data/example.csv > expected.csv
./build/release/replay data/example.csv > actual.csv 2> metrics.json
diff -u expected.csv actual.csv
python3 python/benchmark.py build/release/replay --events 100000 --repeats 3
```

Tests cover book updates, execution boundaries, queue shutdown/backpressure and FIFO drain, exact record-size boundaries, Python comparisons across worker counts, and the CLI workflow/error/metrics contract. Integrated `main` Release and ASan/UBSan checks passed 5/5 locally. TSan built; unit passed once, but four other checks failed during runtime startup on this WSL host. Full race validation remains blocked. See the [integration evidence](docs/INTEGRATION.md) and [historical rework evidence](docs/REWORK.md).

Local `main` now integrates `shaoyu/rework` and both corrected exercises without squashing. Individual fix commits remain in its ancestry; original faulty practice tags are preserved. See [integration evidence](docs/INTEGRATION.md) and the agent-only investigation explanations in the [interview guide](docs/INTERVIEW.md). No personal user investigation is claimed.

**Limits:** in-memory state, no checkpoint or network feed; queues are bounded but order storage is not. Restart by replaying the original file. More threads are not automatically faster.
