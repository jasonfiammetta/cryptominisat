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

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

message(STATUS "LIB directory is '${CMAKE_INSTALL_LIBDIR}'")
message(STATUS "BIN directory is '${CMAKE_INSTALL_BINDIR}'")

if(POLICY CMP0092)
    # Disable passing /W3 by default on MSVC
    cmake_policy(SET CMP0092 NEW)
endif()

if(POLICY CMP0048)
    #policy for VERSION in cmake 3.0
    cmake_policy(SET CMP0048 NEW)
endif()

if(POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
endif()

if(POLICY CMP0046)
    cmake_policy(SET CMP0046 NEW)
endif()

if(POLICY CMP0026)
    cmake_policy(SET CMP0026 NEW)
endif()

if(POLICY CMP0104)
    cmake_policy(SET CMP0104 NEW)
endif()

# -----------------------------------------------------------------------------
# Provide scripts dir for included cmakes to use
# -----------------------------------------------------------------------------
set(CRYPTOMS_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)

# -----------------------------------------------------------------------------
# Make RelWithDebInfo the default build type if otherwise not set
# -----------------------------------------------------------------------------
set(build_types Debug Release RelWithDebInfo MinSizeRel)
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "You can choose the type of build, options are:${build_types}")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Options are ${build_types}"
        FORCE
    )

    # Provide drop down menu options in cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${build_types})
endif()
message(STATUS "Doing a ${CMAKE_BUILD_TYPE} build")

# -----------------------------------------------------------------------------
# Option to enable/disable assertions
# -----------------------------------------------------------------------------

# Filter out definition of NDEBUG from the default build configuration flags.
# We will add this ourselves if we want to disable assertions
foreach (build_config ${build_types})
    string(TOUPPER ${build_config} upper_case_build_config)
    foreach (language CXX C)
        set(VAR_TO_MODIFY "CMAKE_${language}_FLAGS_${upper_case_build_config}")
        string(REGEX REPLACE "(^| )[/-]D *NDEBUG($| )"
                             " "
                             replacement
                             "${${VAR_TO_MODIFY}}")
        #message("Original (${VAR_TO_MODIFY}) is ${${VAR_TO_MODIFY}} replacement is ${replacement}")
        set(${VAR_TO_MODIFY} "${replacement}" CACHE STRING "Default flags for ${build_config} configuration" FORCE)
    endforeach()
endforeach()

project(cryptominisat5)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
enable_language( CXX )
enable_language( C )
include (GNUInstallDirs)
include (GenerateExportHeader)

# contains some library search cmake scripts
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# generate JSON file of compile commands -- useful for code extension
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# static compilation
option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(STATICCOMPILE "Compile to static executable" OFF)
if (STATICCOMPILE)
    set(BUILD_SHARED_LIBS OFF)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

option(SANITIZE "Use Clang sanitizers. You MUST use clang++ as the compiler for this to work" OFF)
option(LARGEMEM "Allow memory usage to grow to Terabyte values -- uses 64b offsets. Slower, but allows the solver to run for much longer." OFF)
if (LARGEMEM)
    add_definitions(-DLARGE_OFFSETS)
endif()

macro(add_sanitize_option flagname)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}" )
endmacro()

include(CheckCXXCompilerFlag)
include(CheckLinkerFlag)
macro(add_cxx_flag_if_supported flagname)
  check_cxx_compiler_flag("${flagname}" HAVE_FLAG_${flagname})

  if(HAVE_FLAG_${flagname})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flagname}" )
  endif()
endmacro()
macro(add_link_flag_if_supported flagname)
  check_linker_flag(CXX "${flagname}" HAVE_FLAG_${flagname})

  if(HAVE_FLAG_${flagname})
    add_link_options("${flagname}")
  endif()
endmacro()

include(CheckCCompilerFlag)
macro(add_c_flag_if_supported flagname)
  check_c_compiler_flag("${flagname}" HAVE_FLAG_${flagname})

  if(HAVE_FLAG_${flagname})
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flagname}" )
  endif()
endmacro()

macro(add_sanitize_flags)
if (SANITIZE)
    MESSAGE(WARNING " --Using clang sanitizers -- you MUST use clang++ or the compile WILL fail")
    add_compile_options("-fsanitize=address")
    add_link_options("-fsanitize=address")
    # below warns on overflows EVEN when that's OK, because it's well-defined, disabling
    # add_compile_options("-fsanitize=integer")
    add_compile_options("-fsanitize=undefined")
    add_compile_options("-fsanitize=null")
    add_compile_options("-fsanitize=alignment")
    #add_compile_options("-fno-sanitize-recover")
    add_compile_options("-fsanitize=return")
    add_compile_options("-fsanitize=bounds")
    add_compile_options("-fsanitize=float-divide-by-zero")
    add_compile_options("-fsanitize=integer-divide-by-zero")
    #    add_compile_options("-fsanitize=unsigned-integer-overflow")
    add_compile_options("-fsanitize=signed-integer-overflow")
    add_compile_options("-fsanitize=bool")
    add_compile_options("-fsanitize=enum")
    add_compile_options("-fsanitize=float-cast-overflow")
    add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:-D_GLIBCXX_ASSERTIONS>")
endif()
endmacro()

option(ENABLE_ASSERTIONS "Build with assertions enabled" ON)
message(STATUS "build type is ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(ENABLE_ASSERTIONS OFF)
endif()

if (ENABLE_ASSERTIONS)
    # NDEBUG was already removed.
else()
    # Note this definition doesn't appear in the cache variables.
    add_definitions(-DNDEBUG)
    add_cxx_flag_if_supported("-fno-stack-protector")
    add_definitions(-D_FORTIFY_SOURCE=0)
endif()

# Note: O3 gives slight speed increase, 1 more solved from SAT Comp'14 @ 3600s
if (NOT MSVC)
    add_compile_options( -g)
    add_compile_options( -pthread )

    #NOTE: out-satrace19-8373595 has confirmed that O3+flto only hurts compared to O2
    #      on gcc version 7.3.0
    add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:-O2>")
    add_compile_options("$<$<CONFIG:RELEASE>:-O2>")
    add_compile_options("$<$<CONFIG:RELEASE>:-g0>")
    add_compile_options("$<$<CONFIG:DEBUG>:-O0>")

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -O2")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2")
    endif()

else()
    # see https://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx for details
    # /ZI = include debug info
    # /Wall = all warnings

    add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:/O2>")
    add_compile_options("$<$<CONFIG:RELWITHDEBINFO>:/ZI>")

    add_compile_options("$<$<CONFIG:RELEASE>:/O2>")
    add_compile_options("$<$<CONFIG:RELEASE>:/D>")
    add_compile_options("$<$<CONFIG:RELEASE>:/NDEBUG>")

    add_compile_options("$<$<CONFIG:DEBUG>:/Od>")

    if (NOT BUILD_SHARED_LIBS)
        # We statically link to reduce dependencies
        foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            # /MD -- Causes the application to use the multithread-specific
            #        and DLL-specific version of the run-time library.
            #        Defines _MT and _DLL and causes the compiler to place
            #        the library name MSVCRT.lib into the .obj file.
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")

            # /MDd	-- Defines _DEBUG, _MT, and _DLL and causes the application to use the debug multithread-specific and DLL-specific version of the run-time library.
            #          It also causes the compiler to place the library name MSVCRTD.lib into the .obj file.
            if(${flag_var} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MDd")
        endforeach(flag_var)

        # Creates a multithreaded executable (static) file using LIBCMT.lib.
        add_compile_options(/MT)
    endif()

    # buffers security check
    add_compile_options(/GS)

    # Proper warning level
    add_compile_options(/W1)

    # Disable STL used in DLL-boundary warning
    add_compile_options(/wd4251)
    add_compile_options(/D_CRT_SECURE_NO_WARNINGS)

    # Wall is MSVC's Weverything, so annoying unless used from the start
    # and with judiciously used warning disables
    # add_compile_options(/Wall)

    # /Za = only ansi C98 & C++11
    # /Za is not recommended for use, not tested, etc.
    # see: http://stackoverflow.com/questions/5489326/za-compiler-directive-does-not-compile-system-headers-in-vs2010
    # add_compile_options(/Za)

    add_compile_options(/fp:precise)

    # exception handling. s = The exception-handling model that catches C++ exceptions only and tells the compiler to assume that functions declared as extern "C" may throw an exception.
    # exception handling. c = If used with s (/EHsc), catches C++ exceptions only and tells the compiler to assume that functions declared as extern "C" never throw a C++ exception.
    add_compile_options(/EHsc)


    # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL:NO")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PDBCOMPRESS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:1572864")

    #what does this do?
    set(DEF_INSTALL_CMAKE_DIR CMake)
endif()

option(ENABLE_TESTING "Enable testing" OFF)

if (NOT WIN32)
    if(NOT ENABLE_TESTING AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        add_cxx_flag_if_supported("-fvisibility=hidden")
    endif()

    add_cxx_flag_if_supported("-msse4.2")
    add_cxx_flag_if_supported("-mpopcnt")
    add_cxx_flag_if_supported("-mpclmul")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      # add_cxx_flag_if_supported("-flto")
      # add_c_flag_if_supported("-flto")
      # add_link_flag_if_supported("-flto")
    else()
        add_cxx_flag_if_supported("-Wall")
        add_cxx_flag_if_supported("-Wextra")
        add_cxx_flag_if_supported("-Wunused")
        add_cxx_flag_if_supported("-Wsign-compare")
        add_cxx_flag_if_supported("-fno-omit-frame-pointer")
        add_cxx_flag_if_supported("-Wtype-limits")
        add_cxx_flag_if_supported("-Wuninitialized")
        add_cxx_flag_if_supported("-Wno-deprecated")
        add_cxx_flag_if_supported("-Wstrict-aliasing")
        add_cxx_flag_if_supported("-Wpointer-arith")
        add_cxx_flag_if_supported("-Wheader-guard")
        add_cxx_flag_if_supported("-Wpointer-arith")
        add_cxx_flag_if_supported("-Wformat-nonliteral")
        add_cxx_flag_if_supported("-Winit-self")
        add_cxx_flag_if_supported("-Wparentheses")
        add_cxx_flag_if_supported("-Wunreachable-code")
        add_cxx_flag_if_supported("-g")
        add_cxx_flag_if_supported("-Wno-class-memaccess")
        add_cxx_flag_if_supported("-Wextra-semi-stmt")
        add_cxx_flag_if_supported("-Wnoweak-vtables")
        add_cxx_flag_if_supported("-ggdb3")

        # Apparently needed before OS X Maverics (2013)
        #add_c_flag_if_supported("-stdlib=libc++")
    endif()
endif()

option(IPASIR "Also build IPASIR" OFF)
option(LIMITMEM "*Only used for testing*. Limit memory used by CMS through number of variables" OFF)
if (LIMITMEM)
    add_definitions(-DLIMITMEM)
endif()

# -----------------------------------------------------------------------------
# Uncomment these for static compilation under Linux (messes up Valgrind)
# -----------------------------------------------------------------------------
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND NOT SANITIZE)
    set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} -Wl,--discard-all -Wl,--build-id=sha1")
endif()

if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
    if(NOT BUILD_SHARED_LIBS)
        MESSAGE(STATUS "Compiling statically")
        if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
        endif()
        SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    else()
        MESSAGE(STATUS "Compiling for dynamic library use")
    endif()
endif()

if(NOT MSVC)
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/cryptominisat5)
endif()

option(SLOW_DEBUG "Use more debug flags" OFF)
IF(SLOW_DEBUG)
    add_definitions(-DSLOW_DEBUG)
endif()

# -----------------------------------------------------------------------------
# Add GIT version
# -----------------------------------------------------------------------------
function(SetVersionNumber PREFIX VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
  set(${PREFIX}_VERSION_MAJOR ${VERSION_MAJOR} PARENT_SCOPE)
  set(${PREFIX}_VERSION_MINOR ${VERSION_MINOR} PARENT_SCOPE)
  set(${PREFIX}_VERSION_PATCH ${VERSION_PATCH} PARENT_SCOPE)
  set(${PREFIX}_VERSION
        "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
        PARENT_SCOPE)
endfunction()

find_program (GIT_EXECUTABLE git)
if (GIT_EXECUTABLE)
  include(GetGitRevisionDescription)
  get_git_head_revision(GIT_REFSPEC GIT_SHA1)
  MESSAGE(STATUS "GIT hash found: ${GIT_SHA1}")
else()
  set(GIT_SHA "GIT-hash-notfound")
endif()

set(CMS_FULL_VERSION "5.13.0")

string(REPLACE "." ";" CMS_FULL_VERSION_LIST ${CMS_FULL_VERSION})
SetVersionNumber("PROJECT" ${CMS_FULL_VERSION_LIST})
MESSAGE(STATUS "PROJECT_VERSION: ${PROJECT_VERSION}")
MESSAGE(STATUS "PROJECT_VERSION_MAJOR: ${PROJECT_VERSION_MAJOR}")
MESSAGE(STATUS "PROJECT_VERSION_MINOR: ${PROJECT_VERSION_MINOR}")
MESSAGE(STATUS "PROJECT_VERSION_PATCH: ${PROJECT_VERSION_PATCH}")

option(FINAL_PREDICTOR "Use final predictor" OFF)
if (FINAL_PREDICTOR)
    message(STATUS "You HAVE to build xgboost and LightGBM with 'cmake -DBUILD_STATIC_LIB=ON -DUSE_OPENMP=OFF ..' for static linking")
    find_package(dmlc REQUIRED)
    find_package(rabit REQUIRED)
    find_package(xgboost REQUIRED)
    find_library(lightgbm
    NAMES _lightgbm lightgbm LightGBM
    REQUIRED)
    add_definitions( -DFINAL_PREDICTOR )
endif()

if (STATS)
    find_package (SQLITE3 REQUIRED)
    find_package(louvain_communities CONFIG REQUIRED)
    MESSAGE(STATUS "OK, Found SQLITE3!")
    include_directories(${SQLITE3_INCLUDE_DIR})
    add_definitions( -DUSE_SQLITE3 )
endif()

option(STATS "Don't use statistics at all" OFF)
if (STATS)
    if (FINAL_PREDICTOR)
        message(FATAL_ERROR "Cannot have stats and final predictor on both")
    endif()
    set(STATS_NEEDED ON)
    add_definitions( -DSTATS_NEEDED )
    if (NOT SQLITE3_FOUND)
        message(FATAL_ERROR "SQLite needed for STATS")
    endif()
ELSE ()
    MESSAGE(STATUS "Not compiling detailed statistics. The system is faster without them")
ENDIF ()

if (STATS)
    find_package(louvain_communities CONFIG)
    if (louvain_communities_FOUND)
        message(STATUS "Found Community Louvain library")
        message(STATUS "Community Louvain dynamic lib: ${COMMLOUVAIN_LIBRARIES}")
        message(STATUS "Community Louvain include dirs: ${COMMLOUVAIN_INCLUDE_DIRS}")
    else()
        message(STATUS "Cannot find Community Louvain. Please install it!")
        message(FATAL_ERROR "For STATS we must have louvain communities installed!")
    endif()
endif()

# ----------
# manpage
# ----------
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND NOT EMSCRIPTEN)
    find_program(HELP2MAN_FOUND help2man)
    if (HELP2MAN_FOUND)
        ADD_CUSTOM_TARGET(man_cryptominisat5
            ALL
            DEPENDS cryptominisat5-bin
        )

        ADD_CUSTOM_COMMAND(
            POST_BUILD
            TARGET man_cryptominisat5
            COMMAND help2man
            ARGS --version-string=${CMS_FULL_VERSION} --help-option="-h" $<TARGET_FILE:cryptominisat5-bin> -o ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5.1
        )

        INSTALL(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/cryptominisat5.1
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)

        message(STATUS "Manpage will be created and installed")
    else()
        MESSAGE(STATUS "Cannot find help2man, not creating manpage")
    endif()
else()
    MESSAGE(STATUS "Not on Linux, not creating manpage")
endif()

# -----------------------------------------------------------------------------
# Look for ZLIB (For reading zipped CNFs)
# -----------------------------------------------------------------------------
option(NOZLIB "Don't use zlib" OFF)

# cannot currently compile static zlib under Windows
if (NOT NOZLIB AND NOT ((NOT BUILD_SHARED_LIBS) AND WIN32))
    find_package(ZLIB)
    IF (ZLIB_FOUND)
        MESSAGE(STATUS "OK, Found ZLIB!")
        include_directories(${ZLIB_INCLUDE_DIR})
        link_directories(${ZLIB_LIB_DIR})
        add_definitions( -DUSE_ZLIB )
    ELSE (ZLIB_FOUND)
        MESSAGE(STATUS "WARNING: Did not find ZLIB, gzipped file support will be disabled")
    ENDIF (ZLIB_FOUND)
endif()


find_library(cadical
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../cadical/build/
    NAMES cadical
    REQUIRED)
find_library(cadiback
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../cadiback/
    NAMES cadiback
    REQUIRED)

include(CheckFloatPrecision)
check_float_precision()
if (HAVE__FPU_SETCW)
    add_definitions(-DYALSAT_FPU)
    message(STATUS "Found FPU code for yalsat: fpu_control.h, _FPU_SINGLE, _FPU_DOUBLE")
endif()

# -----------------------------------------------------------------------------
# MIT option
# -----------------------------------------------------------------------------
option(MIT "Build with only MIT licensed components" OFF)

# -----------------------------------------------------------------------------
# Look for BreakID
# -----------------------------------------------------------------------------
option(NOBREAKID "Disable BreakID" ON)
option(NOMPI "Disable MPI" ON)
if (NOT NOMPI)
    find_package(MPI)
    if (MPI_FOUND)
        add_definitions( -DUSE_MPI )
        set(NOBREAKID ON)
        #set( CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER} )
    else (MPI_FOUND)
         MESSAGE(STATUS "No suitable C++ MPI implementation found. CryptoMiniSat will not be distributed.")
    endif(MPI_FOUND)
endif()

# -----------------------------------------------------------------------------
# Look for BreakID
# -----------------------------------------------------------------------------
if (NOT NOBREAKID)
    find_package(breakid)
    if (NOT BREAKID_LIBRARIES)
        message(WARNING "BreakID NOT found")
    else()
        message(STATUS "BreakID -- found version ${BREAKID_VERSION_MAJOR}.${BREAKID_VERSION_MINOR}")
        message(STATUS "BreakID -- libraries: ${BREAKID_LIBRARIES}")
        message(STATUS "BreakID -- include dirs: ${BREAKID_INCLUDE_DIRS}")
        add_definitions(-DUSE_BREAKID)
    endif()
endif()
find_package(GMP REQUIRED)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

macro(cmsat_add_public_header LIBTARGET HEADER)
    get_target_property(EXISTING_PUBLIC_HEADERS ${LIBTARGET} PUBLIC_HEADER)
    if(EXISTING_PUBLIC_HEADERS)
        list(APPEND EXISTING_PUBLIC_HEADERS "${HEADER}")
    else()
        # Do not append to empty list
        set(EXISTING_PUBLIC_HEADERS "${HEADER}")
    endif()
    set_target_properties(
        ${LIBTARGET}
        PROPERTIES
        PUBLIC_HEADER "${EXISTING_PUBLIC_HEADERS}"
     )
endmacro()

# -----------------------------------------------------------------------------
# Look for MPI
# -----------------------------------------------------------------------------
option(NOMPI "Disable MPI" ON)
if (NOT NOMPI)
    find_package(MPI)
    if (MPI_FOUND)
        add_definitions( -DUSE_MPI )
        set(NOBREAKID ON)
        #set( CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER} )
    else (MPI_FOUND)
         MESSAGE(STATUS "No suitable C++ MPI implementation found. CryptoMiniSat will not be distributed.")
    endif(MPI_FOUND)
endif()

if (FINAL_PREDICTOR OR STATS OR ENABLE_TESTING)
    find_package(Python3 COMPONENTS NumPy Interpreter Development REQUIRED)
    if (Python3_FOUND AND Python3_Interpreter_FOUND AND Python3_NumPy_FOUND)
        message(STATUS "Python 3 -- Python3_EXECUTABLE=${Python3_EXECUTABLE}")
        message(STATUS "Python 3 -- Python3_LIBRARIES=${Python3_LIBRARIES}")
        message(STATUS "Python 3 -- Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}")
        message(STATUS "Python 3 -- Python3_VERSION=${Python3_VERSION}")
        message(STATUS "Python 3 -- Python3_NumPy_INCLUDE_DIRS=${Python3_NumPy_INCLUDE_DIRS}")
        message(STATUS "Python 3 -- Python3_NumPy_VERSION=${Python3_NumPy_VERSION}")
    endif()
endif()

# -----------------------------------------------------------------------------
# Provide an export name to be used by targets that wish to export themselves.
# -----------------------------------------------------------------------------
set(CRYPTOMINISAT5_EXPORT_NAME "cryptominisat5Targets")

#query definitions
get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
set(COMPILE_DEFINES)
foreach( d ${DirDefs} )
    # message( STATUS "Found Define: " ${d} )
    set(COMPILE_DEFINES "${COMPILE_DEFINES} -D${d}")
endforeach()
message(STATUS "All defines at startup: ${COMPILE_DEFINES}")


# -----------------------------------------------------------------------------
# Add uninstall target for makefiles
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------
if (ENABLE_TESTING)
    add_subdirectory(utils/gtest)
endif()
add_subdirectory(src cmsat5-src)
if (ENABLE_TESTING)
    enable_testing()

    message(STATUS "Testing is enabled")
    set(UNIT_TEST_EXE_SUFFIX "Tests" CACHE STRING "Suffix for Unit test executable")
    add_subdirectory(tests)
    add_subdirectory(scripts/fuzz)
    add_subdirectory(utils/minimal_cms)
    add_subdirectory(utils/lingeling-ala)
else()
    message(WARNING "Testing is disabled")
endif()

# -----------------------------------------------------------------------------
# Export our targets so that other CMake based projects can interface with
# the build of cryptominisat5 in the build-tree
# -----------------------------------------------------------------------------
set(CRYPTOMINISAT5_TARGETS_FILENAME "cryptominisat5Targets.cmake")
set(CRYPTOMINISAT5_CONFIG_FILENAME "cryptominisat5Config.cmake")
set(CRYPTOMINISAT5_VERSION_FILENAME "cryptominisat5ConfigVersion.cmake")
set(CRYPTOMINISAT5_STATIC_DEPS ${SQLITE3_LIBRARIES})

# Export targets
set(MY_TARGETS cryptominisat5)
if (IPASIR)
    set(MY_TARGETS ${MY_TARGETS} ipasircryptominisat5)
endif()
export(
    TARGETS ${MY_TARGETS}
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${CRYPTOMINISAT5_TARGETS_FILENAME}"
)

# Create cryptominisat5Config file
set(EXPORT_TYPE "Build-tree")
set(CONF_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include")
configure_file(cryptominisat5Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${CRYPTOMINISAT5_CONFIG_FILENAME}" @ONLY
)

# Export this package to the CMake user package registry
# Now the user can just use find_package(cryptominisat5) on their system
export(PACKAGE cryptominisat5)

set(DEF_INSTALL_CMAKE_DIR lib/cmake/cryptominisat5)
set(CRYPTOMINISAT5_INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
    "Installation directory for cryptominisat5 CMake files")

# Create cryptominisat5Config file
set(EXPORT_TYPE "installed")
set(CONF_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
configure_file(cryptominisat5Config.cmake.in
   "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT5_CONFIG_FILENAME}" @ONLY
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT5_VERSION_FILENAME}"
  COMPATIBILITY SameMajorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT5_VERSION_FILENAME}"
    DESTINATION "${CRYPTOMINISAT5_INSTALL_CMAKE_DIR}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${CRYPTOMINISAT5_CONFIG_FILENAME}"
    DESTINATION "${CRYPTOMINISAT5_INSTALL_CMAKE_DIR}"
)

# Install the export set for use with the install-tree
install(
    EXPORT ${CRYPTOMINISAT5_EXPORT_NAME}
    DESTINATION "${CRYPTOMINISAT5_INSTALL_CMAKE_DIR}"
)
