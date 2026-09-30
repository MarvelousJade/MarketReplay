# Autonomous exercise completion and local main integration

## Scope, origin, and history

These are deliberately seeded debugging exercises, not production incidents. The coding agent completed them autonomously at the user's request, consulting prepared solutions and reproducing failures independently. No claim is made that the user investigated, changed, or verified the fixes personally.

Starting working tree was clean. Local `main`/`origin/main`: `527af9c`; rework: `6f3dea4`; learning: `800a83d`. There were no solution branches or focused fix commits. Both practice tags existed and remain unchanged.

- Exercise 1 focused fix: `2d0b12d`, on `shaoyu/solution-1` from `shaoyu/practice-1` (`37cda3c`).
- Exercise 2 focused fix: `732e945`, directly on `shaoyu/learning` from `shaoyu/practice-2` (`800a83d`).
- Learning merge: `233f550`, preserving the first fix in corrected learning's ancestry. The exercise-2 staging commit had already restored exercise-1 code; the merge therefore retained its independent fix/test/evidence history without reapplying or cherry-picking the patch.
- Local main rework merge: `e19501b` (non-squashed, `--no-ff`).
- Local main corrected-learning merge: `01c1730` (non-squashed, `--no-ff`). Both useful rework changes and individual fix commits remain ancestors.

No merge conflicts occurred. No resets, rebases, squashes, branch/tag rewrites, or pushes were performed. Historical intentional defects remain only in preserved historical checkpoints, not final code. Runtime source/headers match the known-correct rework; new regression tests and investigation materials were added.

## Failing before / passing after

| Exercise | Actual faulty observation | Regression before | Fix and after |
|---|---|---|---|
| 1 | One/four workers retained quantity 6; `C,0,3,1,0,0,0`; Python expected no orders and four applied operations | Prepared CLI regression failed; new direct Book boundary failed on buy-side exact quantity 4 | `>=` → `>` overfill validation; below/equal/above quantities on both sides and shared level pass. Release 4/4 (6.75 s), ASan/UBSan 4/4 (12.23 s), prepared regression passed |
| 2 | Probe: `accepted=1 delivered=0 sum=0 late_push=0` | Prepared regression failed; new FIFO test failed expecting first item 10 | EOF only when empty, not merely closed. Release 4/4 (6.57 s), ASan/UBSan 4/4 (11.84 s), prepared regression passed |

Detailed symptoms, reproduction commands, root causes, alternatives and invariants: [EXERCISE-1.md](EXERCISE-1.md), [EXERCISE-2.md](EXERCISE-2.md). Combined corrected learning passed Release 5/5 (5.51 s) and both prepared regressions before main integration.

## Final local-main checks

Executed against the integrated runtime/test tree at `01c1730` (subsequent changes are documentation only):

```sh
cmake --preset release && cmake --build --preset release --parallel 2
ctest --preset release
python3 practice/mentor/regression.py 1 build/release/replay
c++ -std=c++20 -pthread -Iinclude practice/shutdown.cpp -o /tmp/market-shutdown
python3 practice/mentor/regression.py 2 /tmp/market-shutdown

python3 python/reference.py data/example.csv > /tmp/market-final-expected.csv
./build/release/replay data/example.csv 2 64 > /tmp/market-final-actual.csv 2> /tmp/market-final-metrics.json
diff -u /tmp/market-final-expected.csv /tmp/market-final-actual.csv

cmake --preset asan && cmake --build --preset asan --parallel 2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --preset asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 practice/mentor/regression.py 1 build/asan/replay

cmake --preset tsan && cmake --build --preset tsan --parallel 2
TSAN_OPTIONS=halt_on_error=1 ctest --preset tsan
```

Actual results:

- Release: **5/5 passed**, 7.44 s. Includes unit, execution boundary, differential (record-size boundaries and worker counts), end-to-end CLI/error/metrics checks, and queue shutdown.
- Both prepared regressions passed; example snapshot diff passed.
- ASan/UBSan: **5/5 passed**, 10.43 s; prepared execution CLI regression also passed with sanitized replay. Queue shutdown is covered by the sanitized CTest executable.
- TSan: build passed; unit passed once (0.96 s), **four other tests failed** on `unexpected memory mapping` at runtime startup; suite 1/5, 1.78 s. The differential wrapper reached case 0, workers 2, where the child exited 66. **Full race validation did not pass**; one successful unit run does not remove the host/runtime limitation.
- Host linker printed `.sframe` warnings while returning successful build exit codes. No hosted CI or production tests were run.

Ancestry verification (all must exit zero):

```sh
git merge-base --is-ancestor 6f3dea4 main
git merge-base --is-ancestor 2d0b12d main
git merge-base --is-ancestor 732e945 main
git diff --exit-code 6f3dea4 -- src include python
git diff 527af9c --check
```

Final audit: the ancestry commands above all passed, both fixes also remain ancestors of `shaoyu/learning`, runtime source/header/Python diff against `shaoyu/rework` was empty, and aggregate whitespace checks passed. README/docs relative Markdown links resolved. Practice tags still point to `37cda3c` and `800a83d`; `origin/main` remains `527af9c`. Final documentation diff was inspected before committing.

## Limitations and learning claims

State remains in memory and can grow beyond queue bounds; no durable checkpoint, live feed, matching engine, production performance evidence, or allocation-failure injection. Stream-exception EOF normalization is not implemented. TSan needs a compatible host; race freedom is unverified. No new benchmark was needed for these correctness fixes.

[INTERVIEW.md](INTERVIEW.md) now provides concise agent-investigation explanations and follow-up questions. Personal interview stories require the user's own independent investigation notes; these prepared examples are not substitutes for experience.

## Subsequent branch cleanup

At the user's request, kept only local `main`. Before deletion, verified that `shaoyu/rework`, `shaoyu/learning`, and `shaoyu/solution-1` were fully merged into `main`, then deleted them with `git branch -d`. No commits or practice tags were removed, and no remote changes were made. Branch names above describe the historical integration process; use commit IDs and practice tags for future reproduction.
