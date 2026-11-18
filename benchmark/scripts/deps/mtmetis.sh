#!/usr/bin/env bash
# =============================================================================
# mt-metis: multithreaded (OpenMP) METIS, static library
# =============================================================================
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib/common.sh"
# shellcheck source=../lib/deps.sh
source "$BENCH_ROOT/scripts/lib/deps.sh"

# mt-metis has no git repository, only release archives
MTMETIS_VERSION="0.7.2"
MTMETIS_URL="https://dlasalle.github.io/mt-metis/releases/mt-metis-$MTMETIS_VERSION.tar.gz"
MTMETIS_SHA256="653b3e82ac986762744f95307bcd563e421150ba891657f9feef215ffd1f8a61"

usage() {
	deps_usage mtmetis "Downloads mt-metis $MTMETIS_VERSION into external/mt-metis and builds it."
}

parse_common_args "$@"
deps_fetch_archive "$MTMETIS_URL" "$MTMETIS_SHA256" external/mt-metis
deps_begin mtmetis external/mt-metis

#   BIG*=1        64-bit vertices, edges, weights and part numbers, as in METIS
#                 (IDXTYPEWIDTH=64). The runner needs the same MTMETIS_64BIT_*
#                 definitions: the types in mtmetis.h depend on them
#   *_PATH        domlib, wildriver and the cut-down METIS bundled in the
#                 archive (paths relative to the sources, as in its ./configure)
#   CMAKE_POLICY_VERSION_MINIMUM
#                 CMakeLists.txt asks for CMake 2.6; newer CMake refuses it
#                 without this
deps_configure \
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
	-DBIGEDGES=1 \
	-DBIGVERTICES=1 \
	-DBIGWEIGHTS=1 \
	-DBIGPARTITIONS=1 \
	-DDOMLIB_PATH=domlib \
	-DWILDRIVER_PATH=wildriver \
	-DMETIS_PATH=metis
# The library only: the mtmetis tool is not needed
cmake --build "$DEP_BUILD" --target mtmetis --parallel "$JOBS"

# libmtmetis.a contains its own old copies of GKlib and part of METIS, with
# the same function names (gk_*, ...) as the real GKlib and METIS in deps/.
# So that they do not mix when the runner is linked, all objects are joined
# into one, and only the API functions (MTMETIS_*, mtmetis_*) stay global.
#
# mtmetis_bin.c (main of the tool) gets into the archive through the *.c
# mask; it needs wildriver, so it is removed
require_cmd ld objcopy ar
cp "$DEP_BUILD/lib/libmtmetis.a" "$DEP_BUILD/libmtmetis_api.a"
ar d "$DEP_BUILD/libmtmetis_api.a" mtmetis_bin.c.o
ld -r --whole-archive "$DEP_BUILD/libmtmetis_api.a" -o "$DEP_BUILD/mtmetis_all.o"
objcopy -w --keep-global-symbol='MTMETIS_*' --keep-global-symbol='mtmetis_*' \
	"$DEP_BUILD/mtmetis_all.o"
rm -f "$DEP_PREFIX/lib/libmtmetis.a"
ar rcs "$DEP_PREFIX/lib/libmtmetis.a" "$DEP_BUILD/mtmetis_all.o"
cp "$DEP_SRC/include/mtmetis.h" "$DEP_PREFIX/include/"

deps_finish
