# Rework audit and verification

## Starting point

Branched `shaoyu/rework` from `527af9c`; initial working tree clean. Existing history contains engine, build/benchmark, and documentation commits; it is retained, not presented as new development. No prior AGENTS.md was present.

Already useful: C++20 parser/book/queue/replay separation, worker-owned state, bounded queues, deterministic sorted output, independent Python depth reconstruction, seeded differential cases, sanitizer presets, benchmark metadata. Baseline release CTest: 2/2 passed, 3.88 seconds locally.

No framework or dependency is needed. Do not replace maps or the blocking queue without profiling evidence. Improvements selected: input failure handling, duplicated percentile logic, CLI acceptance coverage, boundary cases, and learning/evidence documentation. Concurrent replay remains educational rather than a promised speedup.

## Increment 1: input integrity

Acceptance: an already-failed stream throws instead of discarding data; 4095–4098 byte records, LF/CRLF, final unterminated records, and recovery after oversized records match the Python oracle across three worker/queue configurations.

Regression: set failbit on a stream containing valid records. The new unit assertion failed (`check failed: threw`) before the fix. Root cause: oversized-line recovery cleared a preexisting failbit and discarded the first record. Reject failed streams before starting workers. This does not alter healthy CLI input or malformed-line recovery.

Verification: release build and CTest passed 2/2 (4.53 seconds); example CLI snapshot matched Python via `diff -u`; `git diff --check` passed. Linker printed a host `.sframe` warning while returning success; no build failure was observed.

## Increment 2: observable CLI contract

Acceptance: one add/partial-execute/gap/reset workflow has an explicit expected snapshot; defaults and multiple worker counts agree; metrics count every processed event, stay within queue bounds, and have ordered percentile bounds. Empty input, argument errors, missing input, and Linux `/dev/full` output failure are checked.

Replaced the second p99.9 bucket scan with the same integer nearest-rank calculation as p50/p99. Retained existing one-argument calls and JSON keys. Added empty, 1000-sample tail, merged, maximum-count, and invalid-fraction tests. No new dependency or metric storage.

Verification: release configure/build and CTest passed 3/3 (4.92 seconds); `git diff --check` passed. CLI fixture is hand-written rather than derived from the same oracle, giving another independent contract check.

## Fresh verification at the working baseline

Environment: GCC 14.2.1, CMake 4.0.1, Python 3.14.7; WSL2 shared host. No network CI run was observed.

| Commands | Actual result |
|---|---|
| `cmake --preset release && cmake --build --preset release --parallel 2 && ctest --preset release` | 3/3 passed, 4.92 s |
| `cmake --preset asan && cmake --build --preset asan --parallel 2`; `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --preset asan` | 3/3 passed, 9.62 s |
| `cmake --preset tsan && cmake --build --preset tsan --parallel 2`; `TSAN_OPTIONS=halt_on_error=1 ctest --preset tsan` | Build passed; 0/3 tests passed. Each failed at sanitizer startup with `unexpected memory mapping` (0.64 s). No race-freedom claim. |
| Example CLI versus `python3 python/reference.py` | Byte-for-byte diff passed |
| `python3 python/benchmark.py build/release/replay --events 100000 --repeats 3 --output bench-results/rework` | All 12 snapshot hashes agreed; measured samples in BENCHMARKS.md |

Tests are local/synthetic, not production evidence. TSan needs a compatible host. Linker `.sframe` warnings did not change successful build exit status. A first commit attempt was blocked by missing Git identity; subsequent commits use command-local `Coding Agent <agent@localhost>` matching existing agent attribution, without changing global settings.

## Scope and remaining limits

Preserved operations, parser rules, snapshot/JSON formats, threaded ownership, queue backpressure, benchmark tooling, and history. No new runtime dependency, checkpoint store, matching engine, live feed, lock-free queue, or speculative optimization. Documentation now distinguishes historical measurements from this rework, explains three decisions, and avoids fabricated authorship or learner stories.

Unverified: TSan runtime validation, hosted CI, production performance, worker allocation-failure injection, stream-exception EOF normalization, and profiling. Orders can grow without bound; one dispatcher and skewed symbol loads limit scaling. Exercise fixes must be investigated by the learner; prepared regressions and solutions do not count as learner experience.

## Learning branch and completion

`shaoyu/learning` was created from the verified `8d878b5` baseline. Fixture/hint/regression preparation: `b2a7a80`. Independent faulty checkpoints: `shaoyu/practice-1` → `37cda3c`, `shaoyu/practice-2` → `800a83d` (learning tip). Both build and reproduce their stated symptoms; focused regressions fail there and pass with reference behavior. Exercise 2 removes the first defect before introducing its own. Learner instructions: DEBUGGING.md. Solutions and detailed observations exist only in the learning branch's `practice/mentor/`, not in this working checkout.

Prepared reference behavior passed both focused regressions and release CTest 3/3 (5.89 s). This is agent preparation, not learner investigation. Returned to `shaoyu/rework`, rebuilt to replace branch-shared artifacts, and release CTest passed 3/3 (4.74 s). Final ASan/UBSan rebuild and CTest passed 3/3 (8.60 s). Aggregate diff against `527af9c` and final uncommitted diff were inspected; whitespace checks passed. Ancestry checks confirmed both branches retain their respective baselines. Existing `main` and all original commits remain unchanged; nothing was pushed or history-rewritten.

Next learning step: choose a practice tag, branch from it, reproduce symptoms, and save your own hypotheses/evidence before requesting hints. Eventual learner fixes and interview stories remain intentionally pending your investigation.
