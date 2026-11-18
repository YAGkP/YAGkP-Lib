# shellcheck shell=bash disable=SC2034
# Benchmark plan template: cp plans/plan_template.sh plans/<plan>.sh
# Defaults are shown; keep only the lines you change

SET=walshaw         # graph set data/<SET>/ (bash data/graphs_manager.sh --list)
GRAPHS=()           # only these graphs; empty: all
EXCLUDE=()          # graphs to skip

KS=(2 4 8 16 32 64)
SOLVERS=(yagkp)     # yagkp kahip_fast kahip_eco kahip_strong metis_kway metis_recursive scotch
                    # mtmetis_kway mtmetis_recursive kaminpar_fast kaminpar_default kaminpar_eco
THREADS=(1)         # for the multithreaded solvers: yagkp scotch mtmetis_* kaminpar_*;
                    # yagkp with more than 1 needs VARIANT=par
REPEATS=1
IMBALANCE=0.05
VARIANT=par         # build/<VARIANT>/yagkp_bench: par or seq
TIMEOUT=0           # time limit of one run in seconds, the run counts as failed; 0: no limit

# export OMP_PROC_BIND=close OMP_PLACES=cores   # thread pinning (the default)