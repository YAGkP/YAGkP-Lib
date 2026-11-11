# shellcheck shell=bash
# Переменные (BUILD_TYPE, YAGKP_*FLAGS, CMAKE_FLAG_ARGS) заполняются здесь, а читаются вызывающим скриптом
# shellcheck disable=SC2034
# =============================================================================
# Единый источник флагов компиляции для всех режимов сборки.
#
# Флаги нигде больше не задаются, в том числе в CMakeLists.txt.
#
# Использование:
#   source "$YAGKP_ROOT/scripts/lib/flags.sh"
#   load_flags release        # debug | release | profile
#   cmake ... "${CMAKE_FLAG_ARGS[@]}"
#
# Настройка через окружение:
#   YAGKP_MARCH  архитектура для -march (по умолчанию native). Пустая строка
#                отключает -march: бинарь переносим между разными узлами
#                кластера. Пример: YAGKP_MARCH=x86-64-v3 bash scripts/...
# =============================================================================

# -----------------------------------------------------------------------------
# Флаги по режимам
# -----------------------------------------------------------------------------

# Оптимизация, общая для release / profile
_flags_optimize() {
	local march="${YAGKP_MARCH-native}"
	local flags="-O3 -DNDEBUG -funroll-loops"
	if [ -n "$march" ]; then
		flags="$flags -march=$march"
	fi
	# В отчёте использовался ещё -ffast-math. По умолчанию выключен:
	# он меняет семантику операций с плавающей точкой (в т. ч. в сторонних
	# библиотеках). Чтобы включить, добавьте сюда: flags="$flags -ffast-math"
	echo "$flags"
}

# load_flags <режим>: заполняет глобальные переменные
#   BUILD_TYPE        тип сборки CMake (Debug | Release | Profile)
#   YAGKP_CFLAGS      флаги C
#   YAGKP_CXXFLAGS    флаги C++
#   YAGKP_LDFLAGS     флаги компоновки
#   CMAKE_FLAG_ARGS   массив -D аргументов для cmake с этими флагами
load_flags() {
	local mode="$1"
	local opt

	case "$mode" in
		debug)
			BUILD_TYPE="Debug"
			YAGKP_CFLAGS="-O0 -g3"
			YAGKP_LDFLAGS=""
			;;
		release)
			BUILD_TYPE="Release"
			YAGKP_CFLAGS="$(_flags_optimize)"
			YAGKP_LDFLAGS=""
			;;
		profile)
			# Те же оптимизации, что в release, плюс данные для gprof
			BUILD_TYPE="Profile"
			opt="$(_flags_optimize)"
			YAGKP_CFLAGS="$opt -g -pg"
			YAGKP_LDFLAGS="-pg"
			;;
		*)
			die "load_flags: неизвестный режим '$mode' (debug | release | profile)"
			;;
	esac

	YAGKP_CXXFLAGS="$YAGKP_CFLAGS"

	local type_upper
	type_upper="$(echo "$BUILD_TYPE" | tr '[:lower:]' '[:upper:]')"

	# Флаги задаются через CMAKE_<LANG>_FLAGS_<TYPE>, то есть полностью
	# заменяют значения CMake по умолчанию для этого типа сборки
	CMAKE_FLAG_ARGS=(
		"-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
		"-DCMAKE_C_FLAGS_${type_upper}=$YAGKP_CFLAGS"
		"-DCMAKE_CXX_FLAGS_${type_upper}=$YAGKP_CXXFLAGS"
		"-DCMAKE_EXE_LINKER_FLAGS_${type_upper}=$YAGKP_LDFLAGS"
	)
}

# Печать текущих флагов (после load_flags), для логов сборки
print_flags() {
	info "Тип сборки  $BUILD_TYPE"
	info "C/C++       $YAGKP_CXXFLAGS"
	if [ -n "$YAGKP_LDFLAGS" ]; then
		info "Компоновка  $YAGKP_LDFLAGS"
	fi
}