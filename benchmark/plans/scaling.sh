# shellcheck shell=bash disable=SC2034
# Speedup of YAGkP with the number of threads, and how the edge cut changes.
# Only the larger graphs: on small ones the time is too short to measure
SET=walshaw
KS=(2 16 64)
SOLVERS=(yagkp)
THREADS=(1 2 4 8 12 24)
REPEATS=3
