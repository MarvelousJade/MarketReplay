# Exercise 1: exact execution boundary — agent investigation

Origin: intentionally introduced practice defect at `37cda3c` (`shaoyu/practice-1`), not a production incident. This autonomous investigation was performed by the coding agent at the user's request; it is not the user's personal experience. Prepared solutions were consulted and independently checked.

## Reproduction and evidence

From the checkpoint, create `shaoyu/solution-1`, then:

```sh
cmake --preset release && cmake --build --preset release --parallel 2
./build/release/replay practice/execution.csv 1 1
./build/release/replay practice/execution.csv 4 1
python3 python/reference.py practice/execution.csv
python3 practice/mentor/regression.py 1 build/release/replay
```

Actual faulty snapshots with both worker counts retained order/level quantity 6 and reported `C,0,3,1,0,0,0`. Python expected an empty book and `C,0,4,0,0,0,0`. The prepared regression failed. Agreement across worker counts suggested a state/validation issue, not a scheduling-specific symptom; isolated book tests confirmed it.

A new CTest `execution_boundary` tests quantities below, equal to, and above remaining quantity on both sides, with another order at the same price. Before the fix it failed: `execution boundary failed: side=B quantity=4`. This small direct-Book reproduction removes parsing and queue scheduling as explanations.

## Cause, alternatives, and fix

Over-execution validation used `>=` rather than `>` against remaining quantity. Legitimate exact fills were rejected before subtraction, but sequence was consumed. Partial execution worked, leaving six; the exact execution of six was then rejected. This explained both the rejection counter and retained level.

Options: clamp all executions (would hide true overfills); special-case equality with a second removal path (duplicates order/level cleanup); correct the comparison (smallest change preserving atomic overfill rejection). Chosen: reject only quantities strictly greater than remaining. Existing subtraction removes zero orders/levels; other same-price orders must remain untouched.

Verification after the fix: prepared CLI regression passed for one/four workers. Release CTest passed 4/4 (6.75 s); ASan/UBSan passed 4/4 (12.23 s). Both include the new below/equal/above boundary, existing full-fill/overfill checks, exact record-size differential cases, and CLI acceptance checks. TSan is attempted again after integration; no race-validation claim is made here.

History note: `800a83d` had already restored this code as part of staging the second exercise, but was not a focused fix/investigation commit. This fix is recorded once on the preserved first checkpoint, then merged into learning without cherry-picking, squashing, or rewriting history.
