# =============================================================================
# yagkp_bench: запуск любого алгоритма (свой или сторонний) с замером метрик
#
# Сторонние библиотеки (METIS, GKlib, SCOTCH, KaHIP) здесь не собираются:
# их собирают скрипты scripts/deps/*.sh с теми же флагами, что и YAGkP,
# в каталог YAGKP_DEPS_DIR (по умолчанию deps/seq, и для параллельной версии тоже)
# =============================================================================

# -----------------------------------------------------------------------------
# yagkp_import_dep(<цель> <библиотека> <файл .a>)
#   Импортированная статическая библиотека из ${YAGKP_DEPS_DIR}/<библиотека>
#   с её каталогом include/. Если файла нет, сразу понятная ошибка
# -----------------------------------------------------------------------------
function(yagkp_import_dep target lib archive)
    set(prefix "${YAGKP_DEPS_DIR}/${lib}")
    set(path   "${prefix}/lib/${archive}")

    if(NOT EXISTS "${path}")
        message(FATAL_ERROR
            "Не найдена сторонняя библиотека: ${path}\n"
            "Соберите их:  bash scripts/deps/all.sh")
    endif()

    add_library(${target} STATIC IMPORTED)
    set_target_properties(${target} PROPERTIES
        IMPORTED_LOCATION             "${path}"
        INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include")
endfunction()

yagkp_import_dep(dep_gklib     gklib  libGKlib.a)
yagkp_import_dep(dep_metis     metis  libmetis.a)
yagkp_import_dep(dep_scotch    scotch libscotch.a)
yagkp_import_dep(dep_scotcherr scotch libscotcherr.a)
yagkp_import_dep(dep_kahip     kahip  libkahip_static.a)

# KaHIP собран в 64-битном режиме (scripts/deps/kahip.sh): kahip_idx в
# kaHIP_interface.h должен быть int64_t и в нашем коде
set_property(TARGET dep_kahip PROPERTY INTERFACE_COMPILE_DEFINITIONS KAHIP_64BIT)

# Зависимости между ними: METIS использует GKlib, GKlib — libm
target_link_libraries(dep_metis  INTERFACE dep_gklib)
target_link_libraries(dep_gklib  INTERFACE m)
target_link_libraries(dep_scotch INTERFACE dep_scotcherr)

# -----------------------------------------------------------------------------
# Приложение
# -----------------------------------------------------------------------------
add_executable(yagkp_bench
    main.cpp
    benchmark.cpp
    libs_wrappers.cpp
)

target_link_libraries(yagkp_bench PRIVATE
    yagkp
    dep_kahip
    dep_metis
    dep_scotch
)