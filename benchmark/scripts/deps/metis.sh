#!/usr/bin/env bash
# =============================================================================
# METIS. Зависит от GKlib (собирается автоматически)
# =============================================================================
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib/common.sh"
# shellcheck source=../lib/deps.sh
source "$BENCH_ROOT/scripts/lib/deps.sh"

usage() {
	deps_usage metis "Собирает METIS из external/METIS. GKlib собирается автоматически."
}

parse_common_args "$@"
deps_begin metis external/METIS gklib

# METIS ожидает заголовок build/xinclude/metis.h внутри своих исходников
# (его создаёт 'make config'). Каталог build/ в METIS игнорируется git'ом,
# так что подмодуль остаётся чистым. Ширина типов: 32 бита, как int в обёртке
xinclude="$DEP_SRC/build/xinclude"
mkdir -p "$xinclude"
{
	echo "#define IDXTYPEWIDTH 64" # суммы весов рёбер больших графов не помещаются в 32 бита
	echo "#define REALTYPEWIDTH 32"
	cat "$DEP_SRC/include/metis.h"
} >"$xinclude/metis.h"
cp "$DEP_SRC/include/CMakeLists.txt" "$xinclude/"

gklib_prefix="$(deps_prefix gklib)"

# METIS сам добавляет -O3 -march=native -Werror в CMAKE_C_FLAGS; наши флаги
# идут после них (CMAKE_C_FLAGS_RELEASE), поэтому -march берётся наш
deps_configure \
	-DGKLIB_PATH="$gklib_prefix" \
	-DSHARED=OFF
# Только библиотека: утилиты METIS не нужны
cmake --build "$DEP_BUILD" --target metis --parallel "$JOBS"

cp "$DEP_BUILD/libmetis/libmetis.a" "$DEP_PREFIX/lib/"
cp "$xinclude/metis.h" "$DEP_PREFIX/include/"

deps_finish