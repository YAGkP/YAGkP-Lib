#!/usr/bin/env bash
# =============================================================================
# Проверка всех сборок с нуля. Справка: bash scripts/check_builds.sh --help
#
# Каждый режим собирается своим малым скриптом scripts/build/<режим>.sh во
# временный каталог (рабочие build/ не затрагиваются), после чего собранные
# программы запускаются на маленьком тестовом графе. Ошибка в одном режиме
# не останавливает проверку остальных; в конце выводится таблица результатов
# =============================================================================
set -euo pipefail
# shellcheck source=lib/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

ALL_MODES=(debug release profile)

usage() {
	cat <<'USAGE'
Использование: bash scripts/check_builds.sh [варианты] [опции]

Собирает все режимы (debug, release, profile) с нуля во временном каталоге
и запускает собранные программы на тестовом графе.

Варианты (можно оба; по умолчанию --seq):
  --seq          последовательная версия
  --par          параллельная версия

Опции:
  --keep         не удалять временный каталог (логи и сборки) после проверки
  -j N, -jN      число потоков сборки (по умолчанию все ядра)
  -h, --help     показать справку

Код возврата: 0, если все проверки прошли, иначе 1.
USAGE
}

# -----------------------------------------------------------------------------
# Аргументы
# -----------------------------------------------------------------------------
chosen_variants=" "
keep=0
jobs="$(default_jobs)"

while [ $# -gt 0 ]; do
	case "$1" in
		--seq | --par)
			case "$chosen_variants" in
				*" ${1#--} "*) ;;
				*) chosen_variants="$chosen_variants${1#--} " ;;
			esac
			;;
		--keep) keep=1 ;;
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
		*) die "неизвестный аргумент: $1 (справка: --help)" ;;
	esac
	shift
done

[ "$chosen_variants" = " " ] && chosen_variants=" seq "
variants=()
for v in seq par; do
	case "$chosen_variants" in *" $v "*) variants+=("$v") ;; esac
done

# -----------------------------------------------------------------------------
# Временный каталог
# -----------------------------------------------------------------------------
WORK="$(mktemp -d "${TMPDIR:-/tmp}/yagkp-check.XXXXXX")"
LOGS="$WORK/logs"
mkdir -p "$LOGS"

cleanup() {
	if [ "$keep" -eq 1 ]; then
		info "Временный каталог сохранён: $WORK"
	else
		rm -rf "$WORK"
	fi
}
trap cleanup EXIT

export YAGKP_BUILD_ROOT="$WORK/build"

# -----------------------------------------------------------------------------
# Тестовый граф: решётка 20x20 (400 вершин) в формате Matrix Market
# -----------------------------------------------------------------------------
GRAPH="$WORK/grid20.mtx"
awk -v N=20 'BEGIN {
	print "%%MatrixMarket matrix coordinate pattern symmetric"
	print N * N, N * N, 2 * N * (N - 1)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++) {
			v = i * N + j + 1
			if (j + 1 < N) print v + 1, v
			if (i + 1 < N) print v + N, v
		}
}' >"$GRAPH"

# -----------------------------------------------------------------------------
# Проверки собранных программ. Вывод идёт в лог режима
# -----------------------------------------------------------------------------

# Разбиение на 4 части: успешный запуск и все вершины в файле разбиения
check_partition_app() {
	local app="$1/apps/partition/yagkp_partition"
	local out="$WORK/partition.$$"
	"$app" --graph "$GRAPH" --k 4 --output "$out" || return 1
	[ "$(wc -l <"$out")" -eq 400 ] || {
		echo "ожидалось 400 строк в файле разбиения"
		return 1
	}
	rm -f "$out"
}

# Профиль: после запуска должен появиться gmon.out
check_profile() {
	local dir="$WORK/gprof"
	mkdir -p "$dir"
	(cd "$dir" && "$1/apps/partition/yagkp_partition" --graph "$GRAPH" --k 4) || return 1
	[ -s "$dir/gmon.out" ] || {
		echo "gmon.out не создан"
		return 1
	}
	rm -rf "$dir"
}

# run_check <режим> <вариант>: сборка и проверки; 0 — успех
run_check() {
	local mode="$1" variant="$2"
	local dir="$YAGKP_BUILD_ROOT/$mode-$variant"

	bash "$YAGKP_ROOT/scripts/build/$mode.sh" "--$variant" -j "$jobs" || return 1

	case "$mode" in
		debug | release) check_partition_app "$dir" ;;
		profile) check_profile "$dir" ;;
	esac
}

# -----------------------------------------------------------------------------
# Прогон
# -----------------------------------------------------------------------------
step "Проверка сборок во временном каталоге $WORK"

results=()
failed=0

for variant in "${variants[@]}"; do
	for mode in "${ALL_MODES[@]}"; do
		name="$mode-$variant"

		log="$LOGS/$name.log"
		info "$name ..."
		start=$SECONDS
		if run_check "$mode" "$variant" >"$log" 2>&1; then
			results+=("$(printf '%-16s %-6s %4d с' "$name" "OK" $((SECONDS - start)))")
		else
			failed=1
			keep=1 # логи понадобятся
			results+=("$(printf '%-16s %-6s %4d с   лог: %s' "$name" "FAIL" $((SECONDS - start)) "$log")")
		fi
	done
done

step "Результаты"
for line in "${results[@]}"; do
	info "$line"
done

if [ "$failed" -ne 0 ]; then
	die "есть непрошедшие проверки"
fi
ok "все проверки прошли"