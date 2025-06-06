# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

if (NOT WIN32)
    add_cxx_flag_if_supported("-Wno-sign-compare")
endif()

set(GTEST_PREFIX ${PROJECT_SOURCE_DIR}/utils/gtest)
message(STATUS "NOTE: if adding the 'gtest' subdirectory fails, you need to issue 'git submodule init' and 'git submodule update'")
set(GTEST_BOTH_LIBRARIES gtest gtest_main)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
add_definitions( -DCMS_TESTING_ENABLED )

# Add handy macros/functions
include(AddSTPGTest)
include(AddGTestSuite)

# Tests that drive cryptominisat by using cnf files (e.g. smt2, smt and cvc files)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests/cnf-files)

# Needed for fuzz-testing
add_subdirectory(${PROJECT_SOURCE_DIR}/utils/cnf-utils cnf-utils)
add_subdirectory(${PROJECT_SOURCE_DIR}/utils/sha1-sat sha1-sat)
add_subdirectory(${PROJECT_SOURCE_DIR}/utils/minisat minisat)

add_sanitize_flags()
# below is expensive tests, let's not run it
# if (Python3_EXECUTABLE AND ZLIB_FOUND AND NOT STATS)
#     add_subdirectory(${PROJECT_SOURCE_DIR}/utils/minisat_only_elim_and_subsume minisat_only_elim_and_subsume)
#     add_subdirectory(simp-checks)
# endif()

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories(
    #${PROJECT_BINARY_DIR}/cmsat5-src
    ${PROJECT_BINARY_DIR}/include
)

set(cryptoms_lib_link_libs cryptominisat5)

# README test
add_executable(readme_test
    readme_test.cpp
)
target_link_libraries(readme_test
    ${cryptoms_lib_link_libs}
)
add_test (
    NAME readme_test
    COMMAND readme_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# C bindings test
if (NOT SANITIZE)
    add_executable(c_test
        c_test.c
    )
    target_link_libraries(c_test
        ${cryptoms_lib_link_libs}
    )
    add_test (
        NAME c_test
        COMMAND c_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# Multisol test
add_executable(multisol_test
    multisol_test.cpp
)
target_link_libraries(multisol_test
    ${cryptoms_lib_link_libs}
)
add_test (
    NAME multisol_test
    COMMAND multisol_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


if (IPASIR)
    add_executable(ipasir_test
        ipasir_test.cpp
    )
    target_link_libraries(ipasir_test
        ${GTEST_BOTH_LIBRARIES}
        ipasircryptominisat5
    )
    add_test (
        NAME ipasir_test
        COMMAND ipasir_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_executable(ipasir_example
        ipasir_example.c
    )
    target_link_libraries(ipasir_example
        ipasircryptominisat5
    )
    add_test (
        NAME ipasir_example
        COMMAND ipasir_example
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# unit tests
set (MY_TESTS
#     clause_alloc_test
    basic_test
    assump_test
    heap_test
    clause_test
    stp_test
    scc_test
    vrepl_test
    clause_cleaner_test
    distiller_test
    distill_long_with_implicit_test
    subsume_impl_test
    intree_test
    xorfinder_test
    searcher_test
    solver_test
    cardfinder_test
    ternary_resolve_test
    gate_test
    implied_by_test
    lucky_test
    definability_test
    gatefinder_test
    matrixfinder_test
    # gauss_test
#    undefine_test
)

foreach(F ${MY_TESTS})
    add_executable(${F}
        ${F}.cpp
    )
    target_link_libraries(${F}
        ${cryptoms_lib_link_libs}
        ${GTEST_BOTH_LIBRARIES}
    )
    add_test (
        NAME ${F}
        COMMAND ${F}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

# if (FINAL_PREDICTOR)
#     add_executable(ml_perf_test
#         ml_perf_test.cpp
#         ${PROJECT_SOURCE_DIR}/src/cl_predictors.cpp
#     )
#
#     target_link_libraries(ml_perf_test
#         LINK_PUBLIC xgboost dmlc rabit rt
#     )
#
#     add_test (
#         NAME ml_perf_test
#         COMMAND ml_perf_test
#         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     )
# endif()
