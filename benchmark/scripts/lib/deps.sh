# shellcheck shell=bash
# Переменные DEP_* заполняются здесь, а читаются вызывающим скриптом
# shellcheck disable=SC2034
# =============================================================================
# Общая логика скриптов сборки сторонних библиотек (scripts/deps/*.sh).
#
# Раскладка:
#   external/<Lib>              исходники (подмодуль git), не изменяются
#                               (without git: a release archive, see
#                               deps_fetch_archive)
#   build/deps/<вариант>/<lib>  промежуточная сборка
#   deps/<вариант>/<lib>        результат: lib/ и include/
#
# Корни build/ и deps/ переопределяются через BENCH_BUILD_ROOT и BENCH_DEPS_ROOT
#
# Все библиотеки собираются с флагами из flags.sh, теми же, что раннер и YAGkP.
#
# Пересборка автоматическая: в deps/<вариант>/<lib>/.stamp записываются
# флаги, компилятор, коммит подмодуля и хэш скрипта сборки. Если что-то из
# этого изменилось, библиотека пересобирается; иначе сборка пропускается.
#
# Скелет скрипта:
#   deps_begin metis external/METIS      # выйдет сразу, если всё актуально
#   deps_configure -DOPTION=...          # cmake с общими флагами
#   cmake --build "$DEP_BUILD" ...
#   deps_finish
# =============================================================================

# shellcheck source=flags.sh
source "$BENCH_ROOT/scripts/lib/flags.sh"

# deps_prefix <lib>: каталог установки библиотеки для текущего VARIANT
deps_prefix() {
	echo "$BENCH_DEPS_ROOT/$VARIANT/$1"
}

# deps_require <lib>: собрать зависимость, если её ещё нет или она устарела
deps_require() {
	local lib="$1"
	bash "$BENCH_ROOT/scripts/deps/$lib.sh" "--$VARIANT" -j "$JOBS"
}

# Содержимое штампа: всё, от чего зависит результат сборки
_deps_stamp_content() {
	local script="$1"
	echo "flags:    $BENCH_CXXFLAGS"
	echo "cc:       $("${CC:-cc}" --version 2>/dev/null | head -n 1)"
	echo "cxx:      $("${CXX:-c++}" --version 2>/dev/null | head -n 1)"
	if [ -n "${DEP_ARCHIVE_SHA256:-}" ]; then
		echo "source:   $DEP_ARCHIVE_SHA256"
	else
		echo "source:   $(git -C "$DEP_SRC" rev-parse HEAD 2>/dev/null || echo unknown)"
	fi
	echo "script:   $(cksum <"$script" | cut -d ' ' -f 1)"
	local dep
	for dep in ${DEP_DEPENDS[@]+"${DEP_DEPENDS[@]}"}; do
		echo "depends:  $dep $(cksum <"$(deps_prefix "$dep")/.stamp" | cut -d ' ' -f 1)"
	done
}

# deps_begin <lib> <путь к исходникам> [зависимость...]
#
# Заполняет:
#   DEP_NAME, DEP_SRC, DEP_BUILD, DEP_PREFIX, DEP_DEPENDS
#   BUILD_TYPE, CMAKE_FLAG_ARGS и прочее из load_flags
#
# Если библиотека уже собрана с теми же параметрами и не задан --clean,
# завершает скрипт с кодом 0
deps_begin() {
	DEP_NAME="$1"
	DEP_SRC="$REPO_ROOT/$2"
	shift 2
	DEP_DEPENDS=("$@")
	DEP_BUILD="$BENCH_BUILD_ROOT/deps/$VARIANT/$DEP_NAME"
	DEP_PREFIX="$(deps_prefix "$DEP_NAME")"

	if [ "$VARIANT" = "par" ]; then
		die "$DEP_NAME: параллельный вариант сторонних библиотек пока не поддерживается"
	fi

	require_cmd cmake git
	if [ -z "${DEP_ARCHIVE_SHA256:-}" ]; then
		init_submodule "${DEP_SRC#"$REPO_ROOT"/}"
	fi

	local dep
	for dep in ${DEP_DEPENDS[@]+"${DEP_DEPENDS[@]}"}; do
		deps_require "$dep"
	done

	load_flags

	local script="${BASH_SOURCE[1]}"
	_DEP_STAMP="$(_deps_stamp_content "$script")"

	if [ "$CLEAN" -eq 0 ] && [ -f "$DEP_PREFIX/.stamp" ] &&
		[ "$(cat "$DEP_PREFIX/.stamp")" = "$_DEP_STAMP" ]; then
		info "$DEP_NAME ($VARIANT): актуальна, пропускаю"
		exit 0
	fi

	step "$DEP_NAME ($VARIANT) -> ${DEP_PREFIX#"$BENCH_ROOT"/}"
	print_flags

	rm -rf "$DEP_BUILD" "$DEP_PREFIX"
	mkdir -p "$DEP_BUILD" "$DEP_PREFIX/lib" "$DEP_PREFIX/include"
}

# deps_fetch_archive <url> <sha256> <source path>
#   For libraries without a git repository: download the archive, check its
#   sha256 and unpack it into <source path> (skipped if already there). Call it
#   before deps_begin; the stamp then has the sha256 instead of a commit
deps_fetch_archive() {
	local url="$1" sha="$2" dir="$REPO_ROOT/$3"
	DEP_ARCHIVE_SHA256="$sha"
	if [ -f "$dir/.archive_sha256" ] && [ "$(cat "$dir/.archive_sha256")" = "$sha" ]; then
		return
	fi
	require_cmd curl tar sha256sum
	step "Скачиваю $url"
	local tmp
	tmp="$(mktemp "$REPO_ROOT/external/.archive.XXXXXX")"
	curl -fsSL -o "$tmp" "$url" || die "не удалось скачать $url"
	echo "$sha  $tmp" | sha256sum -c --quiet - || die "$url: sha256 не совпадает"
	rm -rf "$dir"
	mkdir -p "$dir"
	tar -xzf "$tmp" -C "$dir" --strip-components=1
	rm -f "$tmp"
	echo "$sha" >"$dir/.archive_sha256"
}

# deps_configure [-D...]: конфигурация CMake-проекта библиотеки с общими
# флагами и префиксом установки DEP_PREFIX
#
# --no-warn-unused-cli: флаги передаются и для C, и для C++, а библиотеки на
# чистом C (GKlib, METIS) иначе предупреждают о неиспользованных переменных
deps_configure() {
	cmake -S "$DEP_SRC" -B "$DEP_BUILD" --no-warn-unused-cli \
		"${CMAKE_FLAG_ARGS[@]}" \
		-DCMAKE_INSTALL_PREFIX="$DEP_PREFIX" \
		-DCMAKE_INSTALL_LIBDIR=lib \
		"$@"
}

# deps_finish: записать штамп, сообщить об успехе
deps_finish() {
	echo "$_DEP_STAMP" >"$DEP_PREFIX/.stamp"
	ok "$DEP_NAME собрана в ${DEP_PREFIX#"$BENCH_ROOT"/}"
}

# deps_usage <lib> <описание>: справка, общая для всех scripts/deps/*.sh
deps_usage() {
	cat <<EOF
Использование: bash scripts/deps/$1.sh [опции]

$2
Результат: deps/<вариант>/$1/{lib,include}. Пропускается, если уже собрана
с теми же флагами, компилятором и версией исходников.

EOF
	common_args_help
	echo "  (--clean: пересобрать, даже если библиотека актуальна)"
}