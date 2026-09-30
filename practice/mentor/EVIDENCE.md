# Preparation evidence (spoilers)

Correct baseline `8d878b5`: both focused regressions passed; release CTest 3/3 passed (4.04 s). Regression commands are in SOLUTIONS.md.

Exercise 1 preparation: build succeeded. Focused regression failed with outstanding quantity 6 and counters `C,0,3,1,0,0,0` rather than the expected empty book and four applied operations. Release CTest: unit and differential failed, CLI passed (1/3). The unchanged baseline had passed the same focused regression. This is an intentionally failing learning checkpoint, not a verified working implementation. These checks establish prepared fixtures, not learner experience.
