# Interview cheat sheet

## 30-second explanation

> “I built a C++20 market-data replay engine. One reader parses events and routes each symbol to a worker through a bounded queue. Each worker owns its order books, so book updates need no locks. Sequence gaps make a book untrusted until a reset arrives. I check results against a separate Python implementation and benchmark throughput and latency.”

Use this as a learning summary—not a claim of experience you cannot explain.

## Read the code in this order (~20 minutes)

| File | What to understand |
|---|---|
| `include/market/book.hpp` | Event, Order, Book: the data model |
| `src/book.cpp` | Add, execute, cancel, reset |
| `include/market/queue.hpp` | Mutex, condition variables, full/empty waits |
| `src/replay.cpp` | Route → process → close → join → combine |
| `tests/unit.cpp` | Concrete examples of correctness |

`parser.cpp` validates input; `main.cpp` only handles CLI and I/O. Python is a correctness oracle, not part of the fast path.

## Questions you should answer

**Why threads?** Different symbols can be processed independently. A symbol stays on one worker to preserve order.

**Why not lock every book?** Worker ownership avoids shared writes. Locks are only needed where events cross threads: the queues.

**What is backpressure?** A full queue blocks the producer instead of growing memory or dropping events. A slow worker can therefore slow the reader.

**Why a condition variable?** Threads sleep while waiting. The wait predicate handles spurious wakeups. Close wakes blocked threads so shutdown cannot leave them waiting forever.

**What is RAII here?** Objects own resources: `unique_ptr` owns worker state and `jthread` owns thread lifetime. Queues close and threads join before dependent state is destroyed.

**Why deterministic?** Per-symbol FIFO order is preserved; symbols do not interact; final output is sorted. Thread scheduling changes timing, not book contents.

**Why maps?** Simple ordered storage and O(log n) updates. They allocate nodes and have pointer-chasing costs. A hash table plus ordered price levels could be a later measured optimization.

**How do you know it works?** Hand-written book/queue tests plus byte-for-byte Python comparisons, including malformed records, sequence gaps, resets, and different worker counts. Sanitizers check additional memory/UB/race problems.

**Why not lock-free?** A mutex queue is easier to verify. Only replace it after profiling shows it matters; lock-free does not automatically mean faster.

**What did benchmarking teach you?** More workers were slower in the local smoke run. Cheap updates can cost less than thread handoff. Profiling is needed to identify the cause—not guess it.

**Biggest limits?** One dispatcher, hot-symbol imbalance, unbounded book storage, no durable checkpoint. It reconstructs orders; it does not perform matching or live trading.

## Five-minute practice

1. Trace add 50 → execute 10 → cancel: remaining quantities are 50 → 40 → 0.
2. Explain what happens when sequence 4 follows 2: untrusted until reset.
3. Explain how closing a full queue releases a blocked producer.
4. Run the example with 1 and 4 workers and compare snapshots.
5. Change an expected quantity in a test and confirm the test fails; restore it afterward.
