# Agent workflow

Keep MarketReplay small, understandable, and interview-ready. Preserve useful functionality; add dependencies or rebuild components only for concrete reasons. Explain important decisions plainly.

Before work: read repository instructions and documentation, inspect code/tests/history and Git status, preserve user changes, and make a short checklist with acceptance criteria. Work on a separate branch; never push or rewrite history without authorization.

For each coherent increment: implement → test → investigate failures → fix → verify → inspect diff → commit relevant code, tests, and documentation. Reproduce behavioral bugs and add failing regression tests. Fix mistakes within the uncommitted increment; use separate fix commits for previously committed behavior. Use descriptive messages and report behavior, verification, and commit hash. Never commit secrets/unrelated changes or claim unrun checks passed.

Complete one end-to-end workflow before expanding. Maintain concise run instructions, architecture, actual verification evidence, engineering alternatives/tradeoffs, and known limitations. Distinguish local synthetic measurements from production evidence.

After the baseline passes, prepare two or three plausible defects on a separate learning branch with reproducible faulty checkpoints. Label simulated reports and hypothetical consequences as practice. Keep learner symptoms/reproduction separate from regression tests, diagnosis, hints, and solutions; reveal those only when requested. Keep the implementation branch passing. Verify eventual learner fixes with regression tests.

Interview material must describe the actual implementation, not invent authorship or investigation experience. Prepare a 60-second introduction and three-minute walkthrough. After the learner investigates, derive stories from their notes: problem → hypothesis → evidence → decision → fix → verification.

## Current rework checklist

- [x] Reject unusable input streams; preserve bounded-line recovery and deterministic snapshots.
- [x] Share percentile calculation; verify CLI snapshots, errors, and metric bounds.
- [x] Record fresh release/sanitizer checks, benchmark evidence, decisions, and interview guide.
- [x] Prepare two separate reproducible practice checkpoints; return to `shaoyu/rework`.

Primary checks: `cmake --preset release`, `cmake --build --preset release --parallel 2`, `ctest --preset release`. ASan/UBSan and TSan use their respective presets. Benchmark only release builds. Instructions and evidence live in `docs/`.
