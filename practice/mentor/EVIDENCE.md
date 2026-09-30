# Preparation evidence (spoilers)

Correct baseline `8d878b5`: both focused regressions passed; release CTest 3/3 passed (4.04 s). Regression commands are in SOLUTIONS.md.

Exercise 1 preparation: build succeeded. Focused regression failed with outstanding quantity 6 and counters `C,0,3,1,0,0,0` rather than the expected empty book and four applied operations. Release CTest: unit and differential failed, CLI passed (1/3). The unchanged baseline had passed the same focused regression. This is an intentionally failing learning checkpoint, not a verified working implementation.

Exercise 2 preparation: build succeeded. Probe printed `accepted=1 delivered=0 sum=0 late_push=0`; focused regression failed. Release CTest 0/3 passed, exposing missing queued work. Restoring the reference queue condition (with exercise-1 defect already removed) made both focused regressions pass and full release CTest pass 3/3 (5.89 s). The queue defect was then reintroduced for its preserved checkpoint. These checks establish prepared fixtures and reference fixes, not learner experience.

## Subsequent autonomous completion

At the user's request, the agent reproduced both defects again, added direct boundary/FIFO regressions that failed before the fixes, and preserved focused fixes `2d0b12d` and `732e945`. Corrected learning and rework were merged into local `main` without squashing. See `docs/EXERCISE-1.md`, `docs/EXERCISE-2.md`, and `docs/INTEGRATION.md` for current evidence and limitations. No personal user investigation is claimed; original faulty practice tags are unchanged.
