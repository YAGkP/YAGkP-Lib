#!/usr/bin/env bash
# =============================================================================
# GKlib: вспомогательная библиотека METIS
# =============================================================================
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib/common.sh"
# shellcheck source=../lib/deps.sh
source "$BENCH_ROOT/scripts/lib/deps.sh"

usage() {
	deps_usage gklib "Собирает GKlib (нужна для METIS) из external/GKlib."
}

parse_common_args "$@"
deps_begin gklib external/GKlib

#   GKLIB_BUILD_APPS=OFF       только библиотека, без утилит
#   OPENMP=OFF                 последовательная версия
deps_configure \
	-DGKLIB_BUILD_APPS=OFF \
	-DOPENMP=OFF \
	-DSHARED=OFF
cmake --build "$DEP_BUILD" --parallel "$JOBS"
cmake --install "$DEP_BUILD"

deps_finish