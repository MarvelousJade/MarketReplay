#!/usr/bin/env python3
import io
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from generate import generate
from reference import reference

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "events.csv"
    cases = [b"", b"\n", b"1,A,R,0,-,0,0", b"x" * 4097, b"x" * 4098 + b"\n1,A,R,0,-,0,0\n",
             b"1,A,R,0,-,0,0\r\n2,A,A,1,B,1,2\r\n3,A,E,1,-,0,3\n5,A,X,1,-,0,0\n6,A,R,0,-,0,0\n",
             b"1,A,R,0,-,0,0\x00\n1,B,R,0,-,0,0\n"]
    example = (Path(__file__).resolve().parents[1] / "data/example.csv").read_bytes().splitlines(keepends=True)
    cases.extend(b"".join(example[:end]) for end in range(1, len(example) + 1))
    for seed in range(10):
        cases.append(b"".join(generate(3000, 7, seed, faults=True)))
    # Arbitrary byte inputs exercise parsing independent of the shared generator.
    rng = random.Random(831)
    cases.append(b"\n".join(bytes(rng.randrange(256) for _ in range(rng.randrange(100))) for _ in range(2000)))
    for index, data in enumerate(cases):
        source.write_bytes(data)
        expected = reference(io.BytesIO(data))
        for workers, capacity in ((1, 1), (2, 7), (4, 64)):
            run = subprocess.run([binary, str(source), str(workers), str(capacity)], capture_output=True, timeout=20)
            assert run.returncode == 0, f"case={index} workers={workers} exit={run.returncode}: {run.stderr.decode(errors='replace')}"
            assert run.stdout.decode() == expected, f"oracle mismatch: case={index} workers={workers}"
            metrics = json.loads(run.stderr)
            assert metrics["queue_high_water"] <= capacity
            assert metrics["events"] == metrics["latency_samples"]
    for args in ([str(source), "0"], [str(source), "257"], [str(source), "2", "0"], [str(source), "oops"], [str(source) + ".missing"]):
        assert subprocess.run([binary, *args], capture_output=True, timeout=5).returncode != 0
print(f"{len(cases)} oracle cases x 3 worker/queue configurations passed")
