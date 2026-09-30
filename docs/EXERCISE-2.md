# Exercise 2: graceful queue shutdown — agent investigation

Origin: intentionally introduced practice defect at `800a83d` (`shaoyu/practice-2`), not a production incident. The coding agent investigated autonomously at the user's request; no personal user investigation is claimed. Prepared solutions were consulted and independently checked.

## Reproduction and evidence

On `shaoyu/learning` at the faulty checkpoint:

```sh
cmake --preset release && cmake --build --preset release --parallel 2
c++ -std=c++20 -pthread -Iinclude practice/shutdown.cpp -o /tmp/market-shutdown
/tmp/market-shutdown
python3 practice/mentor/regression.py 2 /tmp/market-shutdown
```

Actual output: `accepted=1 delivered=0 sum=0 late_push=0`; expected two delivered with sum 30. The prepared regression failed. Filling and closing before consumption reproduced the loss without concurrent scheduling, narrowing the problem to the queue's shutdown contract rather than dispatcher hashing or an intermittent race.

New CTest `queue_shutdown` fills three items, closes twice, refuses a late push, then checks each pending item in FIFO order and repeated EOF. Before the fix it failed: `closed queue failed FIFO drain: expected 10`. Checking order as well as count/sum prevents compensating losses/reordering from hiding a defect.

## Cause, alternatives, and fix

`pop()` returned EOF whenever `closed_` was true, even when items remained. The wait predicate correctly woke on closure, but the post-wait termination condition treated graceful closure as immediate cancellation. Accepted work became inaccessible; replay could then join successfully with incomplete books. Scheduling could mask the symptom if workers emptied queues before closure.

Options: delay close until queues appear empty (racy protocol and potential blocked waiters); introduce a distinct cancellation API (unneeded scope expansion); restore drain-on-close semantics (matches documented contract). Chosen: return EOF only when `items_.empty()`. Keep the wait predicate unchanged so empty readers wake, and keep pushes refusing new work after close. Owners still close and join before destruction.

After the fix: the prepared probe regression passed. Release CTest passed 4/4 (6.57 s); ASan/UBSan passed 4/4 (11.84 s). Existing unit checks also cover a blocked producer released by close, empty reader, idempotence, and multiple producers/consumers. The new deterministic regression verifies the pending-work FIFO contract. TSan is attempted after integration; passing these checks does not prove race freedom.

This is a single focused fix on learning. The first exercise's focused fix is merged into learning by ancestry, not cherry-picked or duplicated; the original practice tags remain faulty and unchanged for future use.
