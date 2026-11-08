# shellcheck shell=bash disable=SC2034
# Benchmark plan template: cp benchmark/plan_template.sh benchmark/plans/<plan>.sh
# Defaults are shown; keep only the lines you change

SET=walshaw         # graph set data/<SET>/ (bash data/graphs_manager.sh --list)
GRAPHS=()           # only these graphs; empty: all
EXCLUDE=()          # graphs to skip

KS=(2 4 8 16 32 64)
SOLVERS=(yagkp)     # yagkp kahip_fast kahip_eco kahip_strong metis_kway metis_recursive scotch
THREADS=(1)         # for yagkp only; more than 1 needs VARIANT=par
REPEATS=1
IMBALANCE=0.05
VARIANT=par         # build/benchmark-<VARIANT>: par or seq

# export OMP_PROC_BIND=close OMP_PLACES=cores   # thread pinning (the default)