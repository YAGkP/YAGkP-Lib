# shellcheck shell=bash disable=SC2034
# Edge cut and time of YAGkP against the other partitioners, on every graph of
# the set. YAGkP runs single-threaded here; see scaling.sh for threads
SET=walshaw
KS=(2 4 8 16 32 64)
SOLVERS=(yagkp kahip_fast kahip_eco kahip_strong metis_kway metis_recursive scotch)
THREADS=(1)
REPEATS=1
VARIANT=seq