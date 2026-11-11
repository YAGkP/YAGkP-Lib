#!/usr/bin/env bash
# =============================================================================
# Общий скрипт сборки YAGkP. Справка: bash build.sh --help
#
# Своей логики сборки не содержит: по флагам вызывает малые скрипты
# scripts/build/<режим>.sh, каждый из которых можно запускать и отдельно
# =============================================================================
set -euo pipefail
# shellcheck source=scripts/lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/scripts/lib/common.sh"

# Порядок сборки, если указано несколько режимов
ALL_MODES=(debug release profile)

usage() {
	cat <<'USAGE'
Использование: bash build.sh [режимы] [варианты] [опции]

Режимы (можно несколько; по умолчанию --debug):
  -d, --debug       ядро и приложения, отладочная сборка
  -r, --release     ядро, оптимизированная сборка
  -p, --profile     ядро, сборка для gprof
  -a, --all         все режимы

Варианты (можно оба; по умолчанию --seq):
  --seq             последовательная версия
  --par             параллельная версия

Опции:
  -c, --clean       удалить каталоги сборки перед сборкой
  -j N, -jN         число потоков сборки (по умолчанию все ядра)
  -h, --help        показать справку

Примеры:
  bash build.sh                  # debug, seq
  bash build.sh -r               # release
  bash build.sh -r -p --par      # release и profile, параллельная версия
  bash build.sh -a --seq --par   # всё, в обоих вариантах
  bash build.sh -r -c -j 8

Результат: build/<режим>-<вариант>/
Каждый режим можно собрать и напрямую: bash scripts/build/<режим>.sh --help
USAGE
}

# -----------------------------------------------------------------------------
# Разбор аргументов
# -----------------------------------------------------------------------------
# Выбранные режимы и варианты хранятся строкой " debug release ... ":
# повтор флага ничего не меняет, порядок флагов не важен
chosen_modes=" "
chosen_variants=" "
clean=0
jobs="$(default_jobs)"

choose_mode() {
	case "$chosen_modes" in
		*" $1 "*) ;;
		*) chosen_modes="$chosen_modes$1 " ;;
	esac
}

choose_variant() {
	case "$chosen_variants" in
		*" $1 "*) ;;
		*) chosen_variants="$chosen_variants$1 " ;;
	esac
}

while [ $# -gt 0 ]; do
	case "$1" in
		-d | --debug) choose_mode debug ;;
		-r | --release) choose_mode release ;;
		-p | --profile) choose_mode profile ;;
		-a | --all) for m in "${ALL_MODES[@]}"; do choose_mode "$m"; done ;;
		--seq) choose_variant seq ;;
		--par) choose_variant par ;;
		-c | --clean) clean=1 ;;
		-j)
			[ $# -ge 2 ] || die "-j: ожидается число"
			[[ "$2" =~ ^[1-9][0-9]*$ ]] || die "-j: ожидается положительное число, получено '$2'"
			jobs="$2"
			shift
			;;
		-j*)
			[[ "${1#-j}" =~ ^[1-9][0-9]*$ ]] || die "-j: ожидается положительное число, получено '${1#-j}'"
			jobs="${1#-j}"
			;;
		-h | --help)
			usage
			exit 0
			;;
		*) die "неизвестный аргумент: $1 (справка: bash build.sh --help)" ;;
	esac
	shift
done

[ "$chosen_modes" = " " ] && chosen_modes=" debug "
[ "$chosen_variants" = " " ] && chosen_variants=" seq "

# Канонический порядок: seq раньше par, режимы как в ALL_MODES
modes=()
for m in "${ALL_MODES[@]}"; do
	case "$chosen_modes" in *" $m "*) modes+=("$m") ;; esac
done
variants=()
for v in seq par; do
	case "$chosen_variants" in *" $v "*) variants+=("$v") ;; esac
done

# -----------------------------------------------------------------------------
# Сборка
# -----------------------------------------------------------------------------
summary=()
total_start=$SECONDS

for variant in "${variants[@]}"; do
	for mode in "${modes[@]}"; do
		args=("--$variant" -j "$jobs")
		if [ "$clean" -eq 1 ]; then
			args+=(--clean)
		fi

		start=$SECONDS
		if ! bash "$YAGKP_ROOT/scripts/build/$mode.sh" "${args[@]}"; then
			die "сборка $mode-$variant не удалась (повторить отдельно: bash scripts/build/$mode.sh ${args[*]})"
		fi
		summary+=("$(printf '%-16s %4d с' "$mode-$variant" $((SECONDS - start)))")
		echo >&2
	done
done

step "Итог ($((SECONDS - total_start)) с)"
for line in "${summary[@]}"; do
	info "build/$line"
done