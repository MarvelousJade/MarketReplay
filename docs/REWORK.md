# Rework audit and verification

## Starting point

Branched `shaoyu/rework` from `527af9c`; initial working tree clean. Existing history contains engine, build/benchmark, and documentation commits; it is retained, not presented as new development. No prior AGENTS.md was present.

Already useful: C++20 parser/book/queue/replay separation, worker-owned state, bounded queues, deterministic sorted output, independent Python depth reconstruction, seeded differential cases, sanitizer presets, benchmark metadata. Baseline release CTest: 2/2 passed, 3.88 seconds locally.

No framework or dependency is needed. Do not replace maps or the blocking queue without profiling evidence. Improvements selected: input failure handling, duplicated percentile logic, CLI acceptance coverage, boundary cases, and learning/evidence documentation. Concurrent replay remains educational rather than a promised speedup.

## Increment 1: input integrity

Acceptance: an already-failed stream throws instead of discarding data; 4095–4098 byte records, LF/CRLF, final unterminated records, and recovery after oversized records match the Python oracle across three worker/queue configurations.

Regression: set failbit on a stream containing valid records. The new unit assertion failed (`check failed: threw`) before the fix. Root cause: oversized-line recovery cleared a preexisting failbit and discarded the first record. Reject failed streams before starting workers. This does not alter healthy CLI input or malformed-line recovery.

Verification: release build and CTest passed 2/2 (4.53 seconds); example CLI snapshot matched Python via `diff -u`; `git diff --check` passed. Linker printed a host `.sframe` warning while returning success; no build failure was observed.
