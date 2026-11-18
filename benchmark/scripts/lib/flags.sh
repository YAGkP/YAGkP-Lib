# shellcheck shell=bash
# Переменные (BUILD_TYPE, BENCH_*FLAGS, CMAKE_FLAG_ARGS) заполняются здесь, а читаются вызывающим скриптом
# shellcheck disable=SC2034
# =============================================================================
# Единый источник флагов компиляции.
#
# Одни и те же флаги получают раннер, YAGkP и METIS, KaHIP, SCOTCH, чтобы
# сравнение шло в равных условиях. Нигде больше флаги не задаются.
#
# Использование:
#   source "$BENCH_ROOT/scripts/lib/flags.sh"
#   load_flags
#   cmake ... "${CMAKE_FLAG_ARGS[@]}"
#
# Настройка через окружение:
#   BENCH_MARCH  архитектура для -march (по умолчанию native). Пустая строка
#                отключает -march: бинарь переносим между разными узлами
#                кластера. Пример: BENCH_MARCH=x86-64-v3 bash build.sh
# =============================================================================

# load_flags: заполняет глобальные переменные
#   BUILD_TYPE        тип сборки CMake (Release)
#   BENCH_CFLAGS      флаги C
#   BENCH_CXXFLAGS    флаги C++
#   CMAKE_FLAG_ARGS   массив -D аргументов для cmake с этими флагами
load_flags() {
	local march="${BENCH_MARCH-native}"

	BUILD_TYPE="Release"
	BENCH_CFLAGS="-O3 -DNDEBUG -funroll-loops"
	if [ -n "$march" ]; then
		BENCH_CFLAGS="$BENCH_CFLAGS -march=$march"
	fi
	BENCH_CXXFLAGS="$BENCH_CFLAGS"

	# Флаги задаются через CMAKE_<LANG>_FLAGS_RELEASE, то есть полностью
	# заменяют значения CMake по умолчанию
	CMAKE_FLAG_ARGS=(
		"-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
		"-DCMAKE_C_FLAGS_RELEASE=$BENCH_CFLAGS"
		"-DCMAKE_CXX_FLAGS_RELEASE=$BENCH_CXXFLAGS"
	)
}

# Печать текущих флагов (после load_flags), для логов сборки
print_flags() {
	info "Тип сборки  $BUILD_TYPE"
	info "C/C++       $BENCH_CXXFLAGS"
}
