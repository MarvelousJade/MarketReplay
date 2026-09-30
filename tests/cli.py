#!/usr/bin/env python3
"""End-to-end CLI contract: reconstruction, recovery, metrics, and errors."""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())


def run(*args, **kwargs):
    return subprocess.run([binary, *map(str, args)], capture_output=True, timeout=10, **kwargs)


with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "events.csv"
    source.write_bytes(
        b"1,A,R,0,-,0,0\n2,A,A,1,B,10,5\n3,A,E,1,-,0,2\n"
        b"invalid\n5,A,X,1,-,0,0\n6,A,R,0,-,0,0\n7,A,A,2,S,11,4"
    )
    expected = b"S,A,7,1\nO,A,2,S,11,4\nL,A,S,11,4\nC,1,5,0,0,1,1\n"
    for args, workers, capacity in (((), 2, 1024), ((1, 1), 1, 1), ((4, 7), 4, 7)):
        result = run(source, *args)
        assert result.returncode == 0, result.stderr
        assert result.stdout == expected, result.stdout
        metrics = json.loads(result.stderr)
        assert metrics["workers"] == workers and metrics["capacity"] == capacity
        assert metrics["events"] == metrics["latency_samples"] == 6
        assert metrics["malformed"] == 1
        assert 1 <= metrics["queue_high_water"] <= capacity
        assert 0 <= metrics["blocked_pushes"] <= metrics["events"]
        assert 0 <= metrics["latency_p50_upper_ns"] <= metrics["latency_p99_upper_ns"] <= metrics["latency_p999_upper_ns"]
        assert math.isfinite(metrics["seconds"]) and metrics["seconds"] > 0
        assert math.isfinite(metrics["events_per_second"]) and metrics["events_per_second"] > 0
    for args in ((), (source, 1, 1, "extra")):
        result = run(*args)
        assert result.returncode == 2 and not result.stdout and b"usage:" in result.stderr
    for args in ((source, "-1"), (source, "1x"), (source, 257), (source, 2, 0), (source.with_suffix(".missing"),)):
        result = run(*args)
        assert result.returncode == 1 and not result.stdout and b"replay:" in result.stderr
    source.write_bytes(b"")
    result = run(source)
    assert result.returncode == 0 and result.stdout == b"C,0,0,0,0,0,0\n"
    metrics = json.loads(result.stderr)
    assert metrics["events"] == metrics["latency_samples"] == metrics["queue_high_water"] == 0
    assert all(metrics[key] == 0 for key in ("latency_p50_upper_ns", "latency_p99_upper_ns", "latency_p999_upper_ns"))
    if Path("/dev/full").exists():
        with open("/dev/full", "wb") as sink:
            result = subprocess.run([binary, str(source)], stdout=sink, stderr=subprocess.PIPE, timeout=10)
        assert result.returncode == 1 and b"snapshot write failed" in result.stderr
print("CLI workflow, metrics, empty input, argument and output errors passed")
