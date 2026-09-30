# Debugging practice (learner instructions)

Two **invented practice scenarios**, not production incidents. Both were subsequently investigated and fixed autonomously by the coding agent at the user's request. Local `main` contains the rework and both focused fixes. The fully merged rework, learning, and solution branches were subsequently deleted at the user's request, leaving only local `main`. The original faulty tags remain preserved for independent practice. No personal user investigation is claimed.

Completed investigations (spoilers): [exercise 1](EXERCISE-1.md), [exercise 2](EXERCISE-2.md). Integration and final checks: [INTEGRATION.md](INTEGRATION.md). Neither exercise depends on fixing the other.

Start with a clean working tree. Create your own branch from the chosen checkpoint, configure/build release after switching, and investigate before opening `practice/mentor/` (solutions and regression checks). Rebuild when returning to the working branch: ignored build artifacts are shared across Git branches.

## Exercise 1: unexpected outstanding quantity

**Simulated report:** “After a series of executions, the snapshot still shows quantity we expected to be gone.”

**Hypothetical consequence:** downstream exposure totals could be overstated. This project does not place trades.

Checkpoint: `shaoyu/practice-1` tag (on the learning branch history).

```sh
git switch -c my/exercise-1 shaoyu/practice-1
cmake --preset release && cmake --build --preset release --parallel 2
./build/release/replay practice/execution.csv 1 1
python3 python/reference.py practice/execution.csv
```

Compare orders, levels, and counters. Reproduce with four workers. Write down competing explanations and devise a smaller input before changing code. Acceptance: snapshot agrees with reference, and a focused regression proves the quantity/state invariant. The full release suite must pass after your fix.

## Exercise 2: missing work at shutdown

**Simulated report:** “A batch consumer sometimes finishes with fewer records than were accepted. The short diagnostic below reproduces it without scheduler luck.”

**Hypothetical consequence:** an offline batch could report successful completion with incomplete state.

Checkpoint: `shaoyu/practice-2` tag (historical faulty checkpoint, no longer the tip of `shaoyu/learning`).

```sh
git switch -c my/exercise-2 shaoyu/practice-2
cmake --preset release && cmake --build --preset release --parallel 2
c++ -std=c++20 -pthread -Iinclude practice/shutdown.cpp -o /tmp/market-shutdown
/tmp/market-shutdown
ctest --preset release
```

The probe accepts two items, then consumes them during orderly shutdown. Expected: two delivered, sum 30, later pushes refused. Record actual output. Explain the ownership/lifetime contract and make a deterministic regression, not a sleep-based test. Acceptance: all accepted pending items drain FIFO, blocked waiters are released, and release checks pass.

## Notes to collect

1. Simulated report versus what you actually reproduced.
2. Hypotheses, predicted outputs, diagnostic commands and actual evidence.
3. Fix alternatives and the invariant your chosen change restores.
4. Regression failure before / success after, plus affected full checks.
5. Remaining uncertainty (especially unrun sanitizer checks).

Ask for “exercise N hint 1” when stuck; progressively stronger hints and solutions are separate. The agent's completed explanations are available separately; after your own independent practice, share the diff and notes so we can verify it and build a story based on your actual work. Do not represent prepared exercises as real incidents.
