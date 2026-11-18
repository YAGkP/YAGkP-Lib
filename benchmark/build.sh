#!/usr/bin/env bash
# =============================================================================
# Сборка раннера yagkp_bench. Справка: bash build.sh --help
# =============================================================================
set -euo pipefail
# shellcheck source=scripts/lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/scripts/lib/common.sh"
# shellcheck source=scripts/lib/flags.sh
source "$BENCH_ROOT/scripts/lib/flags.sh"

usage() {
	cat <<'USAGE'
Использование: bash build.sh [опции]

Собирает METIS, KaHIP, SCOTCH (scripts/deps/, при первом запуске и при
изменении флагов), YAGkP и раннер yagkp_bench, все с одними и теми же флагами.
Результат: build/<вариант>/yagkp_bench и build/<вариант>/build_info.txt

USAGE
	common_args_help
	echo "  (--clean пересобирает только раннер; библиотеки: scripts/deps/all.sh --clean)"
}

parse_common_args "$@"

# Сторонние библиотеки однопоточные: и для --par используется их
# последовательная сборка deps/seq
bash "$BENCH_ROOT/scripts/deps/all.sh" --seq -j "$JOBS"

require_cmd cmake
load_flags
BUILD_DIR="$BENCH_BUILD_ROOT/$VARIANT"

step "yagkp_bench ($VARIANT) -> ${BUILD_DIR#"$BENCH_ROOT"/}"
print_flags
if [ "$CLEAN" -eq 1 ]; then
	info "Удаляю ${BUILD_DIR#"$BENCH_ROOT"/}"
	rm -rf "$BUILD_DIR"
fi

parallel=OFF
if [ "$VARIANT" = "par" ]; then
	parallel=ON
fi

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
	"${CMAKE_FLAG_ARGS[@]}" \
	-DYAGKP_BUILD_BENCHMARKS=ON -DYAGKP_BUILD_APPS=OFF \
	-DBENCH_DEPS_DIR="$BENCH_DEPS_ROOT/seq" \
	-DYAGKP_PARALLEL="$parallel"
cmake --build "$BUILD_DIR" --target yagkp_bench --parallel "$JOBS"

# -----------------------------------------------------------------------------
# Что собрано: run_benchmark.sh копирует это в info.txt каждого прогона
# -----------------------------------------------------------------------------
commit_of() { # commit_of <каталог>: коммит и пометка о незакоммиченных правках
	local hash
	hash="$(git -C "$1" rev-parse --short HEAD 2>/dev/null)" || {
		echo "unknown"
		return
	}
	if git -C "$1" status --porcelain --untracked-files=no 2>/dev/null | grep -q .; then
		hash="$hash (uncommitted changes)"
	fi
	echo "$hash"
}

{
	echo "built:     $(date)"
	echo "variant:   $VARIANT"
	echo "flags:     $BENCH_CXXFLAGS"
	echo "compiler:  $("${CXX:-c++}" --version 2>/dev/null | head -n 1)"
	echo "project:   $(commit_of "$REPO_ROOT")"
	for lib in gklib metis scotch kahip mtmetis tbb kaminpar; do
		echo "$(printf '%-10s' "$lib:") $(grep '^source:' "$BENCH_DEPS_ROOT/seq/$lib/.stamp" | awk '{print substr($2, 1, 7)}')"
	done
} >"$BUILD_DIR/build_info.txt"

ok "${BUILD_DIR#"$BENCH_ROOT"/}/yagkp_bench"
sed 's/^/    /' "$BUILD_DIR/build_info.txt" >&2
