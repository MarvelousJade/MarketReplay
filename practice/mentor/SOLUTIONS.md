# Spoilers: mentor hints, diagnoses, and reference fixes

Do not open until requested. These are prepared practice solutions, not the learner's investigation. Symptoms and commands are in `docs/DEBUGGING.md`. Give only the requested hint level.

## Exercise 1

- Hint 1: compare counters, not only the remaining order.
- Hint 2: compare an execution smaller than remaining quantity with one equal to it.
- Hint 3: inspect the over-execution rejection boundary.

**Diagnosis:** the faulty checkpoint changes `e.quantity > it->second.quantity` to `>=`. A legitimate full execution is rejected, consumes sequence, and leaves orders/levels unchanged. The partial execution still works.

**Reference fix:** use `>` again in `src/book.cpp`. Equal quantity must be accepted; the existing subtraction and zero-removal path then clears both order and level. Over-execution must still reject atomically. A one-character mistake demonstrates validation boundaries and state consistency, not a contrived crash.

```sh
python3 practice/mentor/regression.py 1 build/release/replay
ctest --preset release
```

The focused regression requires an empty trusted book after partial then full execution, with four applied and zero rejected events; runs with one/four workers. Existing unit tests also cover overfill and exact fill. Do not weaken tests to make the exercise pass.

## Exercise 2

- Hint 1: distinguish accepting new work from delivering already accepted work.
- Hint 2: define queue closure versus cancellation before inspecting the code.
- Hint 3: inspect the exit condition in `pop()` after the wait predicate succeeds.

**Diagnosis:** the faulty checkpoint changes `if (items_.empty())` to `if (closed_ || items_.empty())`. It incorrectly treats closure as immediate cancellation, dropping queued work from the consumer's perspective. Scheduling can mask the bug if the consumer drains before closure. The probe removes scheduling uncertainty by filling, closing, then draining.

**Reference fix:** return EOF only when empty; preserve the predicate `closed_ || !items_.empty()` so close still releases empty readers. Keep `push` refusing new items after close. This restores graceful shutdown rather than silently returning partial results.

```sh
c++ -std=c++20 -pthread -Iinclude practice/shutdown.cpp -o /tmp/market-shutdown
python3 practice/mentor/regression.py 2 /tmp/market-shutdown
ctest --preset release
```

Focused check requires both queued items delivered, sum 30, and post-close pushes refused. Existing unit tests additionally check blocked producer release, empty-reader close, idempotence, and multiple producers/consumers.

## Verification protocol

Build before checking; shared ignored build files can contain a different branch's binaries. For an eventual fix, run the focused regression failing before and passing after, full release checks, then ASan/UBSan. Attempt TSan on a compatible host; local startup failure is not a passing race check. Record your own commands and reasoning, not these example hypotheses. Commit each verified fix on your learner branch without rewriting practice tags.

Checkpoint isolation: exercise 2 restores the exercise-1 code before introducing its own fault. The working `shaoyu/rework` branch contains neither defect. Actual preparation observations are recorded in `practice/mentor/EVIDENCE.md`.
