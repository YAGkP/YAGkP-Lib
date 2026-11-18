# shellcheck shell=bash disable=SC2034
# Speedup of YAGkP on kahip9 with every number of threads from 1 to 40:
#   python3 analysis/scaling.py <run> --solver yagkp
# 2 x Xeon Silver 4310T: 1..10 threads - one socket, 11..20 - both sockets,
# 21..40 - two threads on some cores (hyper-threading). Threads are pinned to
# cores (OMP_PLACES=cores, OMP_PROC_BIND=close)
SET=kahip9
KS=(2 4 8 16 32 64)
SOLVERS=(yagkp)
THREADS=({1..40})
REPEATS=3
VARIANT=par
TIMEOUT=1800
