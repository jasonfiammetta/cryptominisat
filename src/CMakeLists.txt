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

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${LOUVAIN_COMMUNITIES_INCLUDE_DIRS} )
include_directories( SYSTEM ${MPI_INCLUDE_PATH} )
include_directories( SYSTEM ${GMP_INCLUDE_DIR} )
include_directories( "${CMAKE_SOURCE_DIR}/../cadiback/include/" )

if (NOT WIN32)
    add_cxx_flag_if_supported("-Wno-bitfield-constant-conversion")
    #add_cxx_flag_if_supported("-Wduplicated-cond")
    #add_cxx_flag_if_supported("-Wduplicated-branches")
    add_cxx_flag_if_supported("-Wlogical-op")
    add_cxx_flag_if_supported("-Wrestrict")
    add_cxx_flag_if_supported("-Wnull-dereference")
    add_cxx_flag_if_supported("-Wdouble-promotion")
    add_cxx_flag_if_supported("-Wshadow")
    add_cxx_flag_if_supported("-Wformat=2")
    add_cxx_flag_if_supported("-Wextra-semi")
    add_cxx_flag_if_supported("-pedantic")
    #add_cxx_flag_if_supported("-Wdeprecated")

    if (FINAL_PREDICTOR)
    add_cxx_flag_if_supported("-Wno-unused-parameter")
    add_cxx_flag_if_supported("-Wno-unused-but-set-variable")
    endif()
endif()
add_sanitize_flags()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_TESTING)
    add_definitions( -DCMS_TESTING_ENABLED )
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
    message(STATUS "Including gtest dir ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/GitSHA1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp" @ONLY)

if (STATS OR FINAL_PREDICTOR)
    if (NOT Python3_EXECUTABLE)
        MESSAGE(FATAL_ERROR "Unfortunately, the python interpreter is needed for statistics because of SQL text generation")
    endif()

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py cmsat_tablestructure.sql ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/cmsat_tablestructure.sql ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(tablestruct ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp)
endif()

if (FINAL_PREDICTOR)
    include_directories(${Python3_INCLUDE_DIRS})
    include_directories(${Python3_NumPy_INCLUDE_DIRS})
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py src/predict/predictor_short.json ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/src/predict/predictor_short.json ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(pred_short ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp)

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py src/predict/predictor_long.json ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/src/predict/predictor_long.json ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(pred_long ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp)

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py src/predict/predictor_forever.json ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/src/predict/predictor_forever.json ${CRYPTOMS_SCRIPTS_DIR}/xxd-alike.py
    )
    add_custom_target(pred_forever ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp)
endif()

# Needed for PicoSAT trace generation
add_definitions( -DTRACE )

set(cryptoms_lib_files
    cnf.cpp
    probe.cpp
    oracle_use.cpp
    backbone.cpp
    propengine.cpp
    varreplacer.cpp
    clausecleaner.cpp
    occsimplifier.cpp
    gatefinder.cpp
    subsumestrengthen.cpp
    clauseallocator.cpp
    sccfinder.cpp
    solverconf.cpp
    distillerlong.cpp
    distillerlitrem.cpp
    distillerbin.cpp
    distillerlongwithimpl.cpp
    str_impl_w_impl.cpp
    solutionextender.cpp
    completedetachreattacher.cpp
    searcher.cpp
    solver.cpp
    hyperengine.cpp
    subsumeimplicit.cpp
    datasync.cpp
    reducedb.cpp
    intree.cpp
    searchstats.cpp
    xorfinder.cpp
    cardfinder.cpp
    cryptominisat_c.cpp
    sls.cpp
    sqlstats.cpp
    vardistgen.cpp
    ccnr.cpp
    ccnr_cms.cpp
    ccnr_oracle.cpp
    ccnr_oracle_pre.cpp
    lucky.cpp
    get_clause_query.cpp
    gaussian.cpp
    packedrow.cpp
    matrixfinder.cpp
    mpicosat/mpicosat.c
    mpicosat/version.c
    oracle/oracle.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GitSHA1.cpp
)

if (GPU)
    set(cryptoms_lib_files
        ${cryptoms_lib_files}
        gpuShareLib/Utils.cc
        gpuShareLib/Assigs.cu
        gpuShareLib/BaseTypes.cu
        gpuShareLib/Clauses.cu
        gpuShareLib/ClauseUpdates.cu
        gpuShareLib/CorrespArr.cu
        gpuShareLib/GpuClauseSharerImpl.cu
        gpuShareLib/GpuRunner.cu
        gpuShareLib/GpuUtils.cu
        gpuShareLib/Helper.cu
        gpuShareLib/Reported.cu
        gpuShareLib/Reporter.cu
    )
endif()

set(cryptoms_lib_link_libs "")

set(cryptoms_lib_link_libs
        ${cryptoms_lib_link_libs}
)

if (FINAL_PREDICTOR)
    set(cryptoms_lib_files
        ${cryptoms_lib_files}
#         predict/clustering_imp.cpp
        cl_predictors_xgb.cpp
        cl_predictors_py.cpp
        cl_predictors_lgbm.cpp
        cl_predictors_abs.cpp
    )
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs}
        _lightgbm xgboost dmlc rabit rt ${Python3_LIBRARIES})
endif()

if (STATS_NEEDED)
    set(cryptoms_lib_files
        ${cryptoms_lib_files}
        community_finder.cpp
        satzilla_features_calc.cpp
        satzilla_features.cpp
    )
endif()

if (MPI_FOUND)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${MPI_CXX_LIBRARIES})
endif()

if (BREAKID_LIBRARIES)
    include_directories( ${BREAKID_INCLUDE_DIRS} )

    SET(cryptoms_lib_files ${cryptoms_lib_files} cms_breakid.cpp)
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${BREAKID_LIBRARIES})
endif()

if (STATS)
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        sqlitestats.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/sql_tablestructure.cpp
    )
    SET(cryptoms_lib_link_libs ${cryptoms_lib_link_libs} ${SQLITE3_LIBRARIES})
endif ()

if (FINAL_PREDICTOR)
    SET(cryptoms_lib_files ${cryptoms_lib_files}
        ${CMAKE_CURRENT_BINARY_DIR}/pred_short.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/pred_long.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/pred_forever.cpp
    )
endif()

add_library(cryptominisat5
    ${cryptoms_lib_files}
    cryptominisat.cpp
)
# Make sure the exported target has the include directory set
target_include_directories(cryptominisat5 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(cryptominisat5
    PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_property(TARGET cryptominisat5
    PROPERTY CUDA_ARCHITECTURES 35 50 72
)

if (SQLITE3_FOUND AND STATS)
    add_dependencies(cryptominisat5
        tablestruct
    )
endif()

if (FINAL_PREDICTOR)
    add_dependencies(cryptominisat5
        pred_short
        pred_long
        pred_forever
    )
endif()

# indicate that we depend on pthread, and compile in the actual library
target_link_libraries(cryptominisat5
    LINK_PUBLIC ${cryptoms_lib_link_libs}
    LINK_PUBLIC ${LOUVAIN_COMMUNITIES_LIBRARIES}
    LINK_PUBLIC ${CMAKE_THREAD_LIBS_INIT}
    LINK_PUBLIC ${GMP_LIBRARIES}
    LINK_PUBLIC ${cadiback}
    LINK_PUBLIC ${cadical}
)

if (NOT WIN32)
    set_target_properties(cryptominisat5 PROPERTIES
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
else()
    set_target_properties(cryptominisat5 PROPERTIES
        OUTPUT_NAME cryptominisat5win
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

if (IPASIR)
    add_library(ipasircryptominisat5
        ipasir.cpp
        ${cryptoms_lib_files}
        cryptominisat.cpp
    )
    target_link_libraries(ipasircryptominisat5
        LINK_PUBLIC ${cryptoms_lib_link_libs}
        LINK_PUBLIC ${LOUVAIN_COMMUNITIES_LIBRARIES}
        LINK_PUBLIC ${CMAKE_THREAD_LIBS_INIT}
        LINK_PUBLIC ${GMP_LIBRARIES}
        LINK_PUBLIC ${cadiback}
        LINK_PUBLIC ${cadical}
    )
    set_target_properties(ipasircryptominisat5 PROPERTIES
        OUTPUT_NAME ipasircryptominisat5
        PUBLIC_HEADER "${cryptominisat5_public_headers}"
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
    GENERATE_EXPORT_HEADER(ipasircryptominisat5)
    install(TARGETS ipasircryptominisat5
        EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cryptominisat5"
    )
endif()

cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat_c.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/mpicosat/mpicosat.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/cryptominisat.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/solvertypesmini.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/dimacsparser.h )
cmsat_add_public_header(cryptominisat5 ${CMAKE_CURRENT_SOURCE_DIR}/streambuffer.h )

# -----------------------------------------------------------------------------
# Copy public headers into build directory include directory.
# The cryptominisat5Config.cmake we generate in the build directory depends on
# this.
# -----------------------------------------------------------------------------
set(HEADER_DEST "${PROJECT_BINARY_DIR}/include/cryptominisat5")
add_custom_target(CopyPublicHeaders ALL)
get_target_property(cryptominisat5_public_headers cryptominisat5 PUBLIC_HEADER)
foreach(public_header ${cryptominisat5_public_headers})
    get_filename_component(HEADER_NAME ${public_header} NAME)
    add_custom_command(TARGET CopyPublicHeaders PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory
                               "${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E echo
                       "Copying ${HEADER_NAME} to ${HEADER_DEST}"
                       COMMAND ${CMAKE_COMMAND} -E
                           copy_if_different
                           ${public_header}
                           "${HEADER_DEST}"
                      )
endforeach()

GENERATE_EXPORT_HEADER(cryptominisat5)
install(TARGETS cryptominisat5
    EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cryptominisat5"
)

add_executable(cryptominisat5-bin
    main.cpp
    main_common.cpp
    main_exe.cpp
    signalcode.cpp
)

if (MPI_FOUND)
    add_executable(cryptominisat5_mpi-bin
        main_mpi.cpp
        datasyncserver.cpp
    )
endif()

set(cryptoms_exec_link_libs
    cryptominisat5
)

IF (ZLIB_FOUND)
    SET(cryptoms_exec_link_libs ${cryptoms_exec_link_libs} ${ZLIB_LIBRARY})
ENDIF()


##########################
### Deal with binaries
##########################
if (MPI_FOUND)
    set_target_properties(cryptominisat5_mpi-bin PROPERTIES
        OUTPUT_NAME cryptominisat5_mpi
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    target_link_libraries(cryptominisat5_mpi-bin
        ${cryptoms_exec_link_libs}
        ${MPI_C_LIBRARIES}
    )
    install(TARGETS cryptominisat5_mpi-bin
        # EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat5_mpi")
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set_target_properties(cryptominisat5-bin PROPERTIES
        OUTPUT_NAME cryptominisat5
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE
        LINK_FLAGS "-s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS='[\"callMain\", \"ccall\", \"cwrap\", \"FS\", \"print\"]' -s FORCE_FILESYSTEM=1"
    )
else()
    set_target_properties(cryptominisat5-bin PROPERTIES
        OUTPUT_NAME cryptominisat5
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

target_link_libraries(cryptominisat5-bin
    ${cryptoms_exec_link_libs}
)
install(TARGETS cryptominisat5-bin
    # EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
SET(CPACK_PACKAGE_EXECUTABLES "cryptominisat5-bin" "cryptominisat5")
