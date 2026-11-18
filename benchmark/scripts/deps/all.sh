#!/usr/bin/env bash
# =============================================================================
# Все сторонние библиотеки
# =============================================================================
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/../lib/common.sh"

# Список библиотек: при добавлении новой достаточно дописать её сюда
DEPS_ALL=(gklib metis scotch kahip mtmetis tbb kaminpar)

usage() {
	cat <<USAGE
Использование: bash scripts/deps/all.sh [опции]

Собирает все сторонние библиотеки: ${DEPS_ALL[*]}.
Каждая пропускается, если уже собрана с теми же параметрами.

USAGE
	common_args_help
}

parse_common_args "$@"

args=("--$VARIANT" -j "$JOBS")
if [ "$CLEAN" -eq 1 ]; then
	args+=(--clean)
fi

for lib in "${DEPS_ALL[@]}"; do
	bash "$BENCH_ROOT/scripts/deps/$lib.sh" "${args[@]}"
done

ok "все сторонние библиотеки ($VARIANT) в ${BENCH_DEPS_ROOT#"$BENCH_ROOT"/}/$VARIANT/"