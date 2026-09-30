# Agent workflow

Keep MarketReplay small, understandable, and interview-ready. Preserve useful functionality; add dependencies or rebuild components only for concrete reasons. Explain important decisions plainly.

Before work: read repository instructions and documentation, inspect code/tests/history and Git status, preserve user changes, and make a short checklist with acceptance criteria. Work on a separate branch; never push or rewrite history without authorization.

For each coherent increment: implement → test → investigate failures → fix → verify → inspect diff → commit relevant code, tests, and documentation. Reproduce behavioral bugs and add failing regression tests. Fix mistakes within the uncommitted increment; use separate fix commits for previously committed behavior. Use descriptive messages and report behavior, verification, and commit hash. Never commit secrets/unrelated changes or claim unrun checks passed.

Complete one end-to-end workflow before expanding. Maintain concise run instructions, architecture, actual verification evidence, engineering alternatives/tradeoffs, and known limitations. Distinguish local synthetic measurements from production evidence.

After the baseline passes, prepare two or three plausible defects on a separate learning branch with reproducible faulty checkpoints. Label simulated reports and hypothetical consequences as practice. Keep learner symptoms/reproduction separate from regression tests, diagnosis, hints, and solutions; reveal those only when requested. Keep the implementation branch passing. Verify eventual learner fixes with regression tests.

Interview material must describe the actual implementation, not invent authorship or investigation experience. Prepare a 60-second introduction and three-minute walkthrough. After the learner investigates, derive stories from their notes: problem → hypothesis → evidence → decision → fix → verification.

## Commit identity

At the owner's request, use the repository's configured Git identity for all future commits: `Shaoyu <fanshaoyu9@outlook.com>`. Do not override it with `Coding Agent` or rewrite existing authorship. Commit metadata does not imply personal investigation: continue distinguishing agent work from the user's actual experience in documentation.

## Current rework checklist

- [x] Reject unusable input streams; preserve bounded-line recovery and deterministic snapshots.
- [x] Share percentile calculation; verify CLI snapshots, errors, and metric bounds.
- [x] Record fresh release/sanitizer checks, benchmark evidence, decisions, and interview guide.
- [x] Prepare two separate reproducible practice checkpoints; return to `shaoyu/rework`.

## Autonomous completion checklist

- [x] Reproduce both preserved defects and add failing boundary/FIFO regressions.
- [x] Commit one focused fix per exercise, retaining agent-only evidence.
- [x] Merge verified rework and corrected learning into local `main` without squashing.
- [x] Run final Release/ASan checks; attempt TSan and report startup failures.

Completion evidence is in `docs/INTEGRATION.md`. The original faulty tags stay available; local `main` contains corrected code. Fully merged working/learning branches were deleted at the user's request. Do not present agent investigations as personal user experience.

Primary checks: `cmake --preset release`, `cmake --build --preset release --parallel 2`, `ctest --preset release`. ASan/UBSan and TSan use their respective presets. Benchmark only release builds. Instructions and evidence live in `docs/`.
