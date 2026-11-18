#!/usr/bin/env bash
# =============================================================================
# The compilers of the cluster are too old for C++20 (GCC 4.8.5 and 9.5,
# Intel compilers on top of their libstdc++), and there is no CMake. This
# script installs GCC 13, CMake, make and git from conda-forge into the home
# directory (no root needed, about 2 GB) and writes ~/yagkp_env.sh, which
# build.sh and slurm/run.sbatch use:
#
#   bash slurm/setup_toolchain.sh      # once, on the login node with internet
#   source ~/yagkp_env.sh              # before building
#
# The environment is not activated in the conda way on purpose: activation
# sets CFLAGS/CXXFLAGS (-O2, -march=nocona, ...) that CMake would add to the
# flags of the benchmark.
#
# Programs built with this GCC need its libstdc++ and libgomp, newer than the
# system ones. They find them through RPATH (LDFLAGS below), not through
# LD_LIBRARY_PATH: with LD_LIBRARY_PATH system programs such as curl would load
# the libraries of the environment too and break
# =============================================================================
set -euo pipefail

MAMBA_ROOT_PREFIX="${MAMBA_ROOT_PREFIX:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)/external/micromamba}"
MICROMAMBA="$MAMBA_ROOT_PREFIX/bin/micromamba"
ENV_PREFIX="$MAMBA_ROOT_PREFIX/envs/yagkp-build"
ENV_FILE="$HOME/yagkp_env.sh"

if [ ! -x "$MICROMAMBA" ]; then
	echo ">>> Downloading micromamba into $MAMBA_ROOT_PREFIX/bin"
	mkdir -p "$MAMBA_ROOT_PREFIX"
	curl -fsSL https://micro.mamba.pm/api/micromamba/linux-64/latest |
		tar -xj -C "$MAMBA_ROOT_PREFIX" bin/micromamba
fi

if [ ! -d "$ENV_PREFIX" ]; then
	echo ">>> Creating $ENV_PREFIX: GCC 13, CMake, make, git"
	MAMBA_ROOT_PREFIX="$MAMBA_ROOT_PREFIX" "$MICROMAMBA" create -y -p "$ENV_PREFIX" \
		-c conda-forge --override-channels gcc=13 gxx=13 cmake make git
fi

cat >"$ENV_FILE" <<ENV
# Toolchain for YAGkP-Benchmark from conda-forge (slurm/setup_toolchain.sh)
ENV_PREFIX="$ENV_PREFIX"
export PATH="\$ENV_PREFIX/bin:\$PATH"
export CC="\$ENV_PREFIX/bin/gcc" CXX="\$ENV_PREFIX/bin/g++"
# Built programs find libstdc++ and libgomp of this GCC through RPATH; CMake
# takes LDFLAGS when a build directory is configured for the first time
export LDFLAGS="-Wl,-rpath,\$ENV_PREFIX/lib"
ENV

echo ">>> Written $ENV_FILE"
# shellcheck source=/dev/null
source "$ENV_FILE"
g++ --version | head -n 1
cmake --version | head -n 1
git --version
