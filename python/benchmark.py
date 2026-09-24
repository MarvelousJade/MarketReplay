#!/usr/bin/env python3
"""Generate one fixed workload, warm up, verify snapshots, retain raw measurements."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time
from generate import generate


def sha(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--output", type=Path, default=Path("bench-results"))
    parser.add_argument("--events", type=int, default=1000000)
    parser.add_argument("--symbols", type=int, default=32)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--workers", type=int, nargs="+", default=[1, 2, 4])
    parser.add_argument("--capacity", type=int, default=1024)
    args = parser.parse_args()
    if args.events < 1 or args.repeats < 1 or args.capacity < 1 or not 1 <= args.symbols <= 10000 or any(not 1 <= w <= 256 for w in args.workers):
        parser.error("invalid positive count, symbol count, or worker count")
    binary = args.binary.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    source = args.output / "events.csv"
    with source.open("wb") as stream:
        stream.writelines(generate(args.events, args.symbols, args.seed))
    metadata = {
        "timestamp_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "platform": platform.platform(), "python": platform.python_version(),
        "cpu_count": os.cpu_count(),
        "cpu_affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
        "cpu_info": Path("/proc/cpuinfo").read_text() if Path("/proc/cpuinfo").exists() else platform.processor(),
        "binary": str(binary), "binary_sha256": sha(binary), "input_sha256": sha(source),
        "events": args.events, "symbols": args.symbols, "seed": args.seed,
        "capacity": args.capacity, "warmups_per_configuration": 1,
    }
    cache = binary.parent / "CMakeCache.txt"
    if cache.exists():
        metadata["cmake_cache"] = cache.read_text()
    runs, summaries = [], []
    expected_hash = None
    for workers in args.workers:
        measured = []
        for iteration in range(args.repeats + 1):
            command = [str(binary), str(source.resolve()), str(workers), str(args.capacity)]
            start = time.perf_counter()
            run = subprocess.run(command, capture_output=True, check=True, timeout=600)
            wall = time.perf_counter() - start
            snapshot_hash = hashlib.sha256(run.stdout).hexdigest()
            if expected_hash is None:
                expected_hash = snapshot_hash
            if snapshot_hash != expected_hash:
                raise RuntimeError("nondeterministic final snapshot")
            metrics = json.loads(run.stderr)
            if metrics["events"] != args.events or metrics["malformed"]:
                raise RuntimeError("workload was not fully processed")
            metrics.update(iteration=iteration, warmup=iteration == 0, process_wall_seconds=wall,
                           snapshot_sha256=snapshot_hash, command=command)
            runs.append(metrics)
            if iteration:
                measured.append(metrics)
        summary = {"workers": workers,
                   "median_events_per_second": statistics.median(m["events_per_second"] for m in measured),
                   "min_events_per_second": min(m["events_per_second"] for m in measured),
                   "max_events_per_second": max(m["events_per_second"] for m in measured),
                   "median_run_p99_upper_ns": statistics.median(m["latency_p99_upper_ns"] for m in measured)}
        summaries.append(summary)
        print(json.dumps(summary))
    report = args.output / "report.json"
    report.write_text(json.dumps({"metadata": metadata, "summary": summaries, "runs": runs}, indent=2) + "\n")
    print(f"Raw measurements and metadata: {report}")


if __name__ == "__main__":
    main()
