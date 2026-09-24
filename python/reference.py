#!/usr/bin/env python3
"""Independent, single-threaded order dictionary oracle (no incremental depth)."""
import re
import sys
from collections import defaultdict


def parse(raw):
    if raw.endswith(b"\n"):
        raw = raw[:-1]
    if len(raw) > 4096:
        return None
    if raw.endswith(b"\r"):
        raw = raw[:-1]
    try:
        seq, symbol, op, oid, side, price, qty = raw.decode("ascii").split(",")
        if not re.fullmatch(r"[A-Z0-9_]{1,16}", symbol):
            return None
        numbers = (seq, oid, price, qty)
        if any(not re.fullmatch(r"[0-9]{1,4096}", n) for n in numbers):
            return None
        seq, oid, price, qty = map(int, numbers)
        if max(seq, oid, price, qty) > 10**12 or seq == 0 or qty > 10**9:
            return None
        valid = {
            "R": oid == price == qty == 0 and side == "-",
            "A": oid > 0 and price > 0 and qty > 0 and side in ("B", "S"),
            "X": oid > 0 and price == qty == 0 and side == "-",
            "E": oid > 0 and price == 0 and qty > 0 and side == "-",
        }
        if not valid.get(op, False):
            return None
        return seq, symbol, op, oid, side, price, qty
    except (ValueError, UnicodeDecodeError):
        return None


def reference(lines):
    books = {}
    counts = dict.fromkeys(("malformed", "applied", "rejected", "stale", "gaps", "ignored"), 0)
    for raw in lines:
        event = parse(raw)
        if event is None:
            counts["malformed"] += 1
            continue
        seq, symbol, op, oid, side, price, qty = event
        book = books.setdefault(symbol, {"seq": 0, "sync": False, "orders": {}})
        orders = book["orders"]
        if seq <= book["seq"]:
            counts["stale"] += 1
            continue
        if op == "R":
            orders.clear()
            book.update(seq=seq, sync=True)
            counts["applied"] += 1
            continue
        if book["sync"] and seq != book["seq"] + 1:
            book["sync"] = False
            counts["gaps"] += 1
        book["seq"] = seq
        if not book["sync"]:
            counts["ignored"] += 1
            continue
        if op == "A":
            total = sum(o[2] for o in orders.values() if o[:2] == (side, price))
            if oid in orders or total + qty > 2**64 - 1:
                counts["rejected"] += 1
                continue
            orders[oid] = side, price, qty
        else:
            if oid not in orders or (op == "E" and qty > orders[oid][2]):
                counts["rejected"] += 1
                continue
            old_side, old_price, old_qty = orders[oid]
            if op == "X" or qty == old_qty:
                del orders[oid]
            else:
                orders[oid] = old_side, old_price, old_qty - qty
        counts["applied"] += 1
    output = []
    for symbol, book in sorted(books.items()):
        output.append(f"S,{symbol},{book['seq']},{int(book['sync'])}")
        levels = defaultdict(int)
        for oid, (side, price, qty) in sorted(book["orders"].items()):
            output.append(f"O,{symbol},{oid},{side},{price},{qty}")
            levels[side, price] += qty
        for side, price in sorted(levels, key=lambda k: (k[0], -k[1] if k[0] == "B" else k[1])):
            output.append(f"L,{symbol},{side},{price},{levels[side, price]}")
    output.append("C," + ",".join(map(str, counts.values())))
    return "\n".join(output) + "\n"


if __name__ == "__main__":
    with open(sys.argv[1], "rb") as source:
        sys.stdout.write(reference(source))
