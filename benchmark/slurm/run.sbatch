#!/bin/bash
# =============================================================================
# One series of benchmark plans on one whole node of the cluster.
#
#   sbatch slurm/run.sbatch <plan> [<plan>...]
#   sbatch slurm/run.sbatch speedup_kahip9
#
# Run it from the root of YAGkP-Benchmark: the job starts in the directory of
# sbatch. Before it, on the login node:
#   bash slurm/setup_toolchain.sh                      # once: GCC 13 and CMake
#   source ~/yagkp_env.sh
#   bash data/graphs_manager.sh --install kahip9
#   BENCH_MARCH=icelake-server bash build.sh --par
# The output of the job is slurm/logs/<job id>.out, the results are in
# results/ as usual.
# =============================================================================
#SBATCH --job-name=yagkp-bench
#SBATCH --partition=cpu-XeonSilver4310T
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=3-00:00:00
#SBATCH --output=slurm/logs/%j.out

# The compilers of the cluster are too old, so the benchmark is built with
# GCC from conda-forge (slurm/setup_toolchain.sh). The file that sets it up is
# $BENCH_ENV, by default ~/yagkp_env.sh; nothing is done if there is no such file
BENCH_ENV="${BENCH_ENV:-$HOME/yagkp_env.sh}"
if [ -f "$BENCH_ENV" ]; then
	# shellcheck source=/dev/null
	source "$BENCH_ENV"
fi

set -uo pipefail

if [ $# -eq 0 ]; then
	echo "Usage: sbatch slurm/run.sbatch <plan> [<plan>...]" >&2
	exit 1
fi

echo "node: $(hostname), job: ${SLURM_JOB_ID:-none}, plans: $*"
echo "environment: $([ -f "$BENCH_ENV" ] && echo "$BENCH_ENV" || echo "none")"
bash run_benchmark.sh "$@"
