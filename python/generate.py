#!/usr/bin/env python3
"""Seeded workload: bounded live orders, adds/cancels/partial executions."""
import argparse
import random


def generate(count, symbols=32, seed=42, faults=False):
    rng = random.Random(seed)
    sequence = [0] * symbols
    live = [{} for _ in range(symbols)]
    next_id = 1
    for index in range(count):
        s = index if index < symbols else rng.randrange(symbols)
        symbol = f"SYM{s:04d}"
        sequence[s] += 1
        seq = sequence[s]
        orders = live[s]
        if seq == 1 or (faults and index % 197 == 0):
            orders.clear()
            row = f"{seq},{symbol},R,0,-,0,0"
        elif not orders or (len(orders) < 128 and rng.random() < 0.55):
            oid = next_id
            next_id += 1
            qty = rng.randint(1, 1000)
            orders[oid] = qty
            side = rng.choice(("B", "S"))
            price = rng.randint(9900, 10000) if side == "B" else rng.randint(10001, 10100)
            row = f"{seq},{symbol},A,{oid},{side},{price},{qty}"
        else:
            oid = rng.choice(tuple(orders))
            if rng.random() < 0.5:
                del orders[oid]
                row = f"{seq},{symbol},X,{oid},-,0,0"
            else:
                qty = rng.randint(1, orders[oid])
                orders[oid] -= qty
                if not orders[oid]:
                    del orders[oid]
                row = f"{seq},{symbol},E,{oid},-,0,{qty}"
        if faults and index % 83 == 17:
            yield b"malformed,input\n"  # missing sequence forces recovery
        else:
            yield (row + "\n").encode()
        if faults and index % 113 == 19:
            yield (row + "\n").encode()  # duplicate delivery
        if faults and index % 157 == 23:
            sequence[s] += 1
            yield f"{sequence[s]},{symbol},X,1000000000000,-,0,0\n".encode()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("output")
    parser.add_argument("--events", type=int, default=1000000)
    parser.add_argument("--symbols", type=int, default=32)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--faults", action="store_true")
    args = parser.parse_args()
    if args.events < 0 or not 1 <= args.symbols <= 10000:
        parser.error("events >= 0 and symbols in 1..10000 required")
    with open(args.output, "wb") as output:
        output.writelines(generate(args.events, args.symbols, args.seed, args.faults))
