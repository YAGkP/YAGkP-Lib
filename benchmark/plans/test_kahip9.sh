# shellcheck shell=bash disable=SC2034
# Quick check before the long runs: one graph of kahip9, k = 2, every solver
# of compare_kahip9, the multithreaded ones with 1 and 2 threads
SET=kahip9
GRAPHS=(ecology1)
KS=(2)
SOLVERS=(yagkp scotch mtmetis_kway mtmetis_recursive kaminpar_fast kaminpar_default kaminpar_eco
	metis_kway metis_recursive kahip_fast kahip_eco)
THREADS=(1 2)
VARIANT=par
TIMEOUT=300
