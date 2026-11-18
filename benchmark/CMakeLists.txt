set(BENCH_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/seq" CACHE PATH
    "Каталог с собранными сторонними библиотеками")

# -----------------------------------------------------------------------------
# bench_import_dep(<цель> <библиотека> <файл .a>)
#   Импортированная статическая библиотека из ${BENCH_DEPS_DIR}/<библиотека>
#   с её каталогом include/. Если файла нет, сразу понятная ошибка
# -----------------------------------------------------------------------------
function(bench_import_dep target lib archive)
    set(prefix "${BENCH_DEPS_DIR}/${lib}")
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

bench_import_dep(dep_gklib     gklib  libGKlib.a)
bench_import_dep(dep_metis     metis  libmetis.a)
bench_import_dep(dep_scotch    scotch libscotch.a)
bench_import_dep(dep_scotcherr scotch libscotcherr.a)
bench_import_dep(dep_kahip     kahip  libkahip_static.a)
bench_import_dep(dep_mtmetis   mtmetis libmtmetis.a)

# KaHIP собран в 64-битном режиме (scripts/deps/kahip.sh): kahip_idx в
# kaHIP_interface.h должен быть int64_t и в нашем коде
set_property(TARGET dep_kahip PROPERTY INTERFACE_COMPILE_DEFINITIONS KAHIP_64BIT)

# Зависимости между ними: METIS использует GKlib, GKlib — libm
target_link_libraries(dep_metis  INTERFACE dep_gklib)
target_link_libraries(dep_gklib  INTERFACE m)
target_link_libraries(dep_scotch INTERFACE dep_scotcherr)

# SCOTCH is built with POSIX threads, mt-metis with OpenMP. The -fopenmp
# flag of OpenMP_C applies to C code only, so it does not change the runner
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED COMPONENTS C)
target_link_libraries(dep_scotcherr INTERFACE Threads::Threads)
target_link_libraries(dep_mtmetis INTERFACE OpenMP::OpenMP_C m)

# mt-metis is built with 64-bit types (scripts/deps/mtmetis.sh): mtmetis.h
# needs the same definitions in our code
set_property(TARGET dep_mtmetis PROPERTY INTERFACE_COMPILE_DEFINITIONS
    MTMETIS_64BIT_VERTICES MTMETIS_64BIT_EDGES MTMETIS_64BIT_WEIGHTS
    MTMETIS_64BIT_PARTITIONS)

# KaMinPar and TBB (scripts/deps/kaminpar.sh, tbb.sh) install CMake
# packages; KaMinPar::KaMinPar brings its definitions (KAMINPAR_64BIT_*)
# and TBB with it
find_package(TBB CONFIG REQUIRED
    PATHS "${BENCH_DEPS_DIR}/tbb" NO_DEFAULT_PATH)
find_package(KaMinPar CONFIG REQUIRED
    PATHS "${BENCH_DEPS_DIR}/kaminpar" NO_DEFAULT_PATH)

# -----------------------------------------------------------------------------
# Раннер
# -----------------------------------------------------------------------------
add_executable(yagkp_bench
    src/main.cpp
    src/solvers.cpp
)

set_target_properties(yagkp_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_compile_options(yagkp_bench PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
)

target_link_libraries(yagkp_bench PRIVATE
    yagkp
    dep_kahip
    dep_metis
    dep_scotch
    dep_mtmetis
    KaMinPar::KaMinPar
)

message(STATUS "")
message(STATUS "YAGkP-Benchmark")
message(STATUS "  CXX flags    ${CMAKE_CXX_FLAGS_RELEASE}")

message(STATUS "  Deps         ${BENCH_DEPS_DIR}")
message(STATUS "")
