# Engineering decisions in the rework

These are actual rework decisions; the engine architecture predates this branch. No production measurements are available.

## 1. Distinguish bad transport state from bad records

**Problem:** preexisting failbit was cleared by oversized-line recovery, losing a valid row silently.

**Alternatives:** treat every failure as recoverable; reject every malformed row; reject failed streams up front while retaining bounded-record recovery.

**Evidence:** the new unit test failed before the fix. Boundary differential cases at 4095–4098 bytes pass with LF/CRLF and EOF, including valid records after oversized input.

**Choice:** reject failed streams before worker startup. Healthy feeds can still skip malformed records; read errors fail the run. Smallest change, no new reader abstraction.

**Limitations:** stream exceptions are not normalized into EOF handling. Malformed rows may create sequence gaps; trust is not guessed back into existence.

## 2. One mergeable integer percentile implementation

**Problem:** p99.9 had its own rank calculation and bucket scan, duplicating p50/p99 behavior.

**Alternatives:** retain duplicate logic; store/sort every duration for exact percentiles; share a rational nearest-rank function over the existing histogram.

**Evidence:** tests cover empty input, 999/1000 boundary ranks, merged histograms, and uint64 maximum sample count without multiplication overflow. CLI checks percentile ordering.

**Choice:** numerator/denominator API, preserving one-argument percentage calls and existing JSON. Histograms remain 64 counters per worker and combine by addition. No allocation per duration; reported values are upper bounds.

**Limitations:** power-of-two precision, timer overhead on every event, count overflow after an impractically large run. No new precision/performance claim was measured.

## 3. Retain ownership and ordered maps instead of speculative redesign

**Problem:** the existing concurrent design is understandable, but more workers are not faster locally. Should the rework replace queues/maps or introduce a separate synchronous path?

**Alternatives:** lock-free queues; hash-indexed orders with ordered levels; synchronous fast path; retain current design and measure before optimizing.

**Evidence:** deterministic oracle comparisons across worker counts and a fresh 100,000-event benchmark. Medians: 695,308 events/s with one worker, 447,608 with two, 68,526.6 with four. Same snapshot hash in all 12 warmup/measured runs. Not a comparison against synchronous or lock-free implementations.

**Choice:** retain worker ownership, blocking queues, and ordered maps. They provide straightforward lifetime/order reasoning and reproducible output. Do not add a second execution path without profiling and a concrete performance requirement.

**Tradeoffs/limits:** map allocation and thread handoff costs, hot-symbol imbalance, one dispatcher, bounded queues but unbounded books. Scheduling/lock overhead are hypotheses, not measured root causes. TSan remains blocked at runtime on this host; release/ASan success does not prove race freedom.
