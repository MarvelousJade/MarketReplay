#!/usr/bin/env python3
"""Spoiler-side focused regressions; run only after your own investigation."""
from pathlib import Path
import subprocess
import sys

exercise, executable = sys.argv[1], str(Path(sys.argv[2]).resolve())
if exercise == "1":
    source = Path(__file__).resolve().parents[1] / "execution.csv"
    for workers in (1, 4):
        run = subprocess.run([executable, str(source), str(workers), "1"], capture_output=True, timeout=10)
        assert run.returncode == 0, run.stderr
        assert run.stdout == b"S,A,4,1\nC,0,4,0,0,0,0\n", run.stdout
elif exercise == "2":
    run = subprocess.run([executable], capture_output=True, timeout=10)
    assert run.returncode == 0, run.stderr
    assert run.stdout == b"accepted=1 delivered=2 sum=30 late_push=0\n", run.stdout
else:
    raise SystemExit("usage: regression.py 1 REPLAY_BINARY | 2 SHUTDOWN_PROBE")
print(f"exercise {exercise} regression passed")
