#!/usr/bin/env bash
# =============================================================================
# SCOTCH: только последовательная библиотека на C
# =============================================================================
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib/common.sh"
# shellcheck source=../lib/deps.sh
source "$YAGKP_ROOT/scripts/lib/deps.sh"

usage() {
	deps_usage scotch "Собирает SCOTCH из external/scotch."
}

parse_common_args "$@"
deps_begin scotch external/scotch

#   INTSIZE=64                SCOTCH_Num = int64: суммы весов рёбер больших
#                             графов не помещаются в 32 бита
#   BUILD_PTSCOTCH=OFF        без MPI
#   BUILD_FORTRAN=OFF         Fortran-интерфейс не нужен, а компилятора может не быть
#   BUILD_LIBSCOTCHMETIS=OFF  иначе SCOTCH положит свой metis.h и он
#   INSTALL_METIS_HEADERS=OFF   будет конкурировать с настоящим из deps/
#   THREADS=OFF               однопоточный, как и остальные участники сравнения
#   USE_ZLIB/LZMA/BZ2=OFF     иначе libscotch потребует -lz -llzma -lbz2 при линковке
deps_configure \
	-DBUILD_SHARED_LIBS=OFF \
	-DINTSIZE=64 \
	-DBUILD_PTSCOTCH=OFF \
	-DBUILD_FORTRAN=OFF \
	-DBUILD_LIBESMUMPS=OFF \
	-DBUILD_LIBSCOTCHMETIS=OFF \
	-DINSTALL_METIS_HEADERS=OFF \
	-DTHREADS=OFF \
	-DUSE_ZLIB=OFF \
	-DUSE_LZMA=OFF \
	-DUSE_BZ2=OFF \
	-DENABLE_TESTS=OFF
cmake --build "$DEP_BUILD" --parallel "$JOBS"
cmake --install "$DEP_BUILD"

deps_finish