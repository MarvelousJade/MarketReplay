# Input, output, and recovery

## Input: seven CSV fields

```text
sequence,symbol,operation,order_id,side,price_ticks,quantity
```

No header. Example:

```csv
1,XYZ,R,0,-,0,0
2,XYZ,A,100,B,12345,50
3,XYZ,E,100,-,0,10
4,XYZ,X,100,-,0,0
```

This resets XYZ, adds a buy order for 50, executes 10, then cancels the remaining 40.

| Op | Action | Fields |
|---|---|---|
| R | Reset to empty, trusted book | id/price/quantity = 0; side `-` |
| A | Add | positive id/price/quantity; side B (buy) or S (sell) |
| E | Execute quantity | positive id/quantity; price 0; side `-` |
| X | Cancel remaining order | positive id; price/quantity 0; side `-` |

**Validation:** symbols `[A-Z0-9_]{1,16}`; unsigned decimal numbers ≤ 10^12; sequence > 0; quantity ≤ 10^9. Prices are integer ticks. IDs are unique within a symbol's live orders.

Maximum line: 4096 bytes excluding LF, including optional CR. LF/CRLF and an unterminated final line work. No quoting or whitespace trimming. Invalid fields, overflow, embedded NUL, and oversized records are skipped and counted. Oversized lines use fixed reader storage.

## Recovery: remember these rules

1. Start untrusted. A reset establishes a baseline.
2. Sequence ≤ last seen → **stale**, ignore it (even a reset).
3. Newer reset → clear orders, become trusted.
4. Missing sequence → become untrusted; ignore updates until reset.
5. Duplicate ID, unknown ID, or over-execution → reject without changing orders, but consume the sequence.
6. Malformed line → skip it without advancing any symbol's sequence.

Untrusted books retain old orders for diagnostics only. Their output flag is 0. Reset is authoritative—not a guessed repair. Restart recovery means replaying the immutable file from the beginning; there is no durable checkpoint.

## Output

- `S,symbol,last_sequence,trusted_0_or_1`
- `O,symbol,id,side,price,remaining_quantity`
- `L,symbol,side,price,total_quantity`
- `C,malformed,applied,rejected,stale,gaps,ignored`

Symbols and IDs sort ascending; bids sort highest first, asks lowest first. Applied includes resets; gap events also count as ignored. Timings are separate JSON, not part of deterministic output.

Exit 0 means replay completed, **not** that the input was clean. Exit 1 means fatal error; 2 means wrong argument count.

## Important implementation details

- `Book::apply` expects parser-validated events.
- Price-level total = sum of remaining quantities at that price; remove zero levels.
- Semantic errors leave orders unchanged. Allocation errors abort the entire run.
- Closing a queue wakes readers/writers and drains pending work; threads join before their state is destroyed.
- A worker failure closes all queues and is rethrown after joining. No partial result is returned.
- An input stream already carrying failbit or badbit is rejected before starting workers. Previously, clearing failbit as though it meant an oversized record could silently discard the first valid record. Oversized records remain recoverable; I/O failures are fatal.
- The stream API expects ordinary non-throwing iostream state handling (the CLI uses this default). A caller enabling stream exceptions may receive an exception even on normal EOF.
