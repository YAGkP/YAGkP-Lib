# shellcheck shell=bash disable=SC2034
# All partitioners on kahip9: edge cut and time for every k.
#   python3 analysis/bars.py <run>
# The multithreaded ones (yagkp, scotch, mt-metis, KaMinPar) run with 20
# threads, one per physical core of 2 x Xeon Silver 4310T; METIS and KaHIP
# are sequential and run with 1. Without kahip_strong: on these graphs one run
# of it takes from minutes to hours
SET=kahip9
KS=(2 4 8 16 32 64)
SOLVERS=(yagkp scotch mtmetis_kway mtmetis_recursive kaminpar_fast kaminpar_default kaminpar_eco
	metis_kway metis_recursive kahip_fast kahip_eco)
THREADS=(20)
REPEATS=3
VARIANT=par
TIMEOUT=1800
