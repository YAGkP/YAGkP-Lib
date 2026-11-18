# shellcheck shell=bash disable=SC2034
# Speedup of the parallel solvers with the number of threads.
# One solver at a time: python3 analysis/scaling.py <run> --solver <solver>
SET=walshaw
KS=(2 16 64)
SOLVERS=(yagkp scotch mtmetis_kway kaminpar_default)
THREADS=(1 2 4 8 12 24)
REPEATS=3
