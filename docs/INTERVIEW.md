# Interview preparation

These are implementation explanations, not claims that you originally built the project. Rework started from an existing engine. Adapt the wording only after you can trace the code and explain the evidence.

## 60-second introduction

> MarketReplay reconstructs an order book from a CSV feed in C++20; it does not match trades. One reader validates events and routes each symbol to a worker through a bounded queue. Workers own their books, avoiding shared book writes. Per-symbol sequences detect missing data, and a reset restores trust. Output is sorted, so timing can change without changing the final snapshot. The rework preserved this architecture, hardened input failure handling, consolidated histogram calculations, and added explicit CLI acceptance coverage. Verification combines focused C++ tests, an independent Python oracle, and sanitizer runs. Local synthetic benchmarks showed that more workers were slower, so concurrency is not sold as a performance guarantee. The main limits are in-memory order storage, a single dispatcher, and no durable recovery checkpoint.

## Three-minute technical walkthrough

**0:00–0:45 — contract.** Open `data/example.csv` and `src/parser.cpp`. Trace reset → add → execute → cancel. Explain integer ticks and exact seven-field validation. Malformed rows do not advance sequences; missing sequence numbers can therefore invalidate a symbol.

**0:45–1:30 — state.** Open `src/book.cpp`. Orders are keyed by ID; bids/asks aggregate remaining quantities by price. Semantic rejection consumes sequence but must not mutate orders. A gap makes retained data diagnostic-only until an authoritative reset. Show the trust flag in the snapshot.

**1:30–2:15 — ownership and lifetime.** Open `src/replay.cpp` and `include/market/queue.hpp`. Symbol hashing preserves per-symbol FIFO order. A full queue blocks the reader. Close wakes waiters, prevents new pushes, and drains pending work. Join before combining state or destroying dependencies. Worker failure closes all queues and prevents returning a partial result.

**2:15–3:00 — evidence and tradeoffs.** Show `tests/cli.py`, `tests/differential.py`, and `docs/REWORK.md`. Explain the preexisting-failbit regression and nearest-rank histogram bounds. Release and ASan/UBSan passed locally; TSan startup failed, so race validation is incomplete. Compare local benchmark medians without claiming a measured bottleneck. Explain why maps/blocking queues were retained rather than speculatively optimized.

## Read in order

`include/market/book.hpp` → `src/parser.cpp` → `src/book.cpp` → `include/market/queue.hpp` → `src/replay.cpp` → `tests/unit.cpp` → `tests/cli.py`. The CLI handles files and presentation; Python is an oracle, not a runtime dependency for replay.

## Follow-up questions

- Why must one symbol stay on one worker? What changes if symbols interact?
- Why does a rejected operation consume sequence, while malformed input does not?
- How do you prove order quantities and level totals stay consistent?
- What distinguishes queue closure from immediate cancellation? Which threads can wait?
- Why are condition-variable predicates needed? What happens if notifications occur first?
- What does `jthread` provide, and why is explicit queue closure still necessary?
- What does the latency timer include/exclude? Can lower p99 coexist with lower throughput?
- Why merge histograms instead of averaging worker p99s? What precision is lost?
- Would a hash map improve this workload? What experiment would justify the change?
- What would a durable checkpoint need to record to avoid sequence/state inconsistency?

## Completed agent investigations: technical explanations

These intentionally seeded exercises were completed autonomously by the coding agent, not investigated personally by the user. They are not production incidents or claims of original authorship. Full evidence: [exercise 1](EXERCISE-1.md), [exercise 2](EXERCISE-2.md), [integration](INTEGRATION.md).

**Exact-fill boundary:** a partial then exact execution left six outstanding and increased rejection count. A validation-boundary hypothesis was tested by a direct Book regression below/equal/above remaining quantity; equality failed without queues involved. `>=` incorrectly rejected equality. Changing it to `>` reused existing zero-removal logic, preserving atomic overfill rejection. Both sides and another order at the same price passed afterward, along with CLI/oracle and sanitizer checks. Clamping overfills was rejected because it hides invalid events.

**Shutdown data loss:** a deterministic fill → close → drain probe accepted two items but delivered zero. This reproduced without concurrent scheduling, pointing to shutdown semantics rather than requiring a race explanation. `pop()` returned EOF on closure even with pending items. Returning EOF only when empty restored FIFO draining while close still wakes waiters and pushes still refuse new work. Delaying close until queues look empty was rejected as a fragile protocol. FIFO, repeated EOF, idempotence, blocked-waiter and integration checks passed after the fix. ASan/UBSan success is not race validation; TSan runtime startup is blocked locally.

### Exercise follow-up questions

- What should happen below, at, and above remaining quantity, and why must rejection leave both order and level unchanged?
- Why test another order at the same price? Why is an empty-book assertion insufficient for every aggregation bug?
- How did a direct Book reproduction separate validation from parsing and concurrency?
- Why does closing a queue wake a reader even when empty? Why should it still deliver pending items?
- How can scheduling hide the shutdown defect? What makes the fill/close/drain test deterministic?
- How would a separate immediate-cancellation API differ, and what result/error contract would it need?
- Why do Release and ASan passes not prove race freedom? Which local TSan outcome actually occurred?

## Your investigation notes and eventual stories

Use `docs/DEBUGGING.md` to practice independently if desired. Record commands, actual outputs, hypotheses rejected, the smallest fix, and regression results. No personal learner investigation has occurred yet; do not retell the agent's work as your experience.

After each exercise, fill this from your own notes:

- **Problem:** what you observed (separate simulated report from your reproduction).
- **Hypothesis:** what you predicted and how you tried to disprove it.
- **Evidence:** commands/tests and their actual output.
- **Decision:** alternatives and why you chose one.
- **Fix:** what you changed and the invariant restored.
- **Verification:** failing-before/passing-after regression and full affected checks.

Bring these notes back for progressive hints or a concise interview story. Explain reasoning rather than memorize this guide.
