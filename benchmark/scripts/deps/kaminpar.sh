#!/usr/bin/env bash
# =============================================================================
# KaMinPar: multithreaded (TBB) partitioner, the shared-memory library only
# =============================================================================
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib/common.sh"
# shellcheck source=../lib/deps.sh
source "$BENCH_ROOT/scripts/lib/deps.sh"

usage() {
	deps_usage kaminpar "Builds KaMinPar from external/KaMinPar."
}

parse_common_args "$@"
deps_begin kaminpar external/KaMinPar tbb

#   TBB_DIR                         our TBB from deps/, not one downloaded
#                                   by KaMinPar
#   KAMINPAR_64BIT_IDS, _WEIGHTS    64-bit IDs and weights, as in the other
#                                   libraries. The runner gets the same
#                                   definitions from KaMinParConfig.cmake
#   KAMINPAR_BUILD_WITH_MTUNE_NATIVE=OFF
#                                   otherwise KaMinPar adds -march=native on
#                                   top of our flags; flags.sh sets -march
#   KAMINPAR_BUILD_WITH_DEBUG_SYMBOLS=OFF, _CCACHE=OFF
#                                   no -g and no ccache, as for the others
#   KAMINPAR_BUILD_WITH_KASSERT=OFF no assertions (and no kassert download)
#   KAMINPAR_ENABLE_TIMERS=OFF      internal timers are not needed
#   KAMINPAR_BUILD_APPS/IO/TESTS=OFF, BUILD_TESTING=OFF
#                                   the library only
deps_configure \
	-DTBB_DIR="$(deps_prefix tbb)/lib/cmake/TBB" \
	-DKAMINPAR_64BIT_IDS=ON \
	-DKAMINPAR_64BIT_WEIGHTS=ON \
	-DKAMINPAR_BUILD_WITH_MTUNE_NATIVE=OFF \
	-DKAMINPAR_BUILD_WITH_DEBUG_SYMBOLS=OFF \
	-DKAMINPAR_BUILD_WITH_CCACHE=OFF \
	-DKAMINPAR_BUILD_WITH_KASSERT=OFF \
	-DKAMINPAR_ENABLE_TIMERS=OFF \
	-DKAMINPAR_BUILD_APPS=OFF \
	-DKAMINPAR_BUILD_IO=OFF \
	-DKAMINPAR_BUILD_TESTS=OFF \
	-DBUILD_TESTING=OFF \
	-DKAMINPAR_BUILD_DISTRIBUTED=OFF
cmake --build "$DEP_BUILD" --parallel "$JOBS"
cmake --install "$DEP_BUILD"

deps_finish
