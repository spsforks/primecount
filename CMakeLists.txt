cmake_minimum_required(VERSION 3.4)
project(primecount CXX)
set(PRIMECOUNT_VERSION_MAJOR 4)
set(PRIMECOUNT_VERSION_MINOR 4)
set(PRIMECOUNT_VERSION "${PRIMECOUNT_VERSION_MAJOR}.${PRIMECOUNT_VERSION_MINOR}")

# Build options ######################################################

option(WITH_POPCNT        "Enable POPCNT instruction"   ON)
option(WITH_LIBDIVIDE     "Use libdivide.h"             ON)
option(WITH_OPENMP        "Enable OpenMP support"       ON)
option(WITH_MPI           "Enable MPI support"          OFF)
option(BUILD_PRIMECOUNT   "Build primecount binary"     ON)
option(BUILD_SHARED_LIBS  "Build shared libprimecount"  OFF)
option(BUILD_STATIC_LIBS  "Build static libprimecount"  ON)
option(BUILD_TESTS        "Build test programs"         OFF)

if(WIN32)
    set(BUILD_SHARED_LIBS OFF)
endif()

if(NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "One or both of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be set to ON")
endif()

# primecount binary source files #####################################

set(BIN_SRC src/app/cmdoptions.cpp
            src/app/help.cpp
            src/app/main.cpp)

# primecount library source files ####################################

set(LIB_SRC src/FactorTable.cpp
            src/Li.cpp
            src/P2.cpp
            src/P3.cpp
            src/PhiTiny.cpp
            src/PiTable.cpp
            src/S1.cpp
            src/Sieve.cpp
            src/LoadBalancer.cpp
            src/S2Status.cpp
            src/generate.cpp
            src/nth_prime.cpp
            src/phi.cpp
            src/pi_legendre.cpp
            src/pi_lehmer.cpp
            src/pi_meissel.cpp
            src/pi_primesieve.cpp
            src/primecount.cpp
            src/print.cpp
            src/test.cpp
            src/lmo/pi_lmo1.cpp
            src/lmo/pi_lmo2.cpp
            src/lmo/pi_lmo3.cpp
            src/lmo/pi_lmo4.cpp
            src/lmo/pi_lmo5.cpp
            src/lmo/pi_lmo_parallel.cpp
            src/deleglise-rivat/S2_hard.cpp
            src/deleglise-rivat/S2_trivial.cpp
            src/deleglise-rivat/pi_deleglise_rivat1.cpp
            src/deleglise-rivat/pi_deleglise_rivat2.cpp
            src/deleglise-rivat/pi_deleglise_rivat_parallel.cpp)

# Required includes ##################################################

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

# Set default build type to Release ##################################

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

# libprimesieve ######################################################

set(COPY_BUILD_TESTS "${BUILD_TESTS}")
set(BUILD_TESTS OFF CACHE BOOL "Build primesieve tests" FORCE)
option(BUILD_PRIMESIEVE "Build primesieve binary" OFF)
add_subdirectory(lib/primesieve)
set(BUILD_TESTS "${COPY_BUILD_TESTS}" CACHE BOOL "Build test programs" FORCE)

# Silence GCC switch fall through warnings ###########################

check_cxx_compiler_flag(-Wno-implicit-fallthrough Wno_fallthrough)

if(Wno_fallthrough)
    set_source_files_properties(src/Sieve.cpp PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)
endif()

# Check for MPI (Message Passing Interface) ##########################

if(WITH_MPI)
    find_package(MPI REQUIRED)

    if(TARGET MPI::MPI_CXX)
        set(LIB_MPI "MPI::MPI_CXX")
    else()
        include_directories("${MPI_INCLUDE_PATH}")
    endif()

    set(HAVE_MPI "HAVE_MPI")
    set(LIB_SRC ${LIB_SRC}
                src/mpi/P2_mpi.cpp
                src/mpi/S2_hard_mpi.cpp
                src/mpi/MpiLoadBalancer.cpp
                src/mpi/MpiMsg.cpp)
endif()

# Check for OpenMP ###################################################

if(WITH_OPENMP)
    find_package(OpenMP QUIET)

    if(OpenMP_FOUND OR OpenMP_CXX_FOUND)
        cmake_push_check_state()
        set(CMAKE_REQUIRED_FLAGS "${OpenMP_CXX_FLAGS} ${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")
        set(CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include")

        check_cxx_source_compiles("
            #include <int128_t.hpp>
            int main() {
                primecount::maxint_t sum = 0;
                #pragma omp parallel for reduction(+: sum)
                for (int i = 0; i < 100; i++)
                    sum += i;
                return 0;
            }" OpenMP)

        if(NOT OpenMP)
            find_library(LIB_ATOMIC NAMES atomic libatomic.so.1)

            if(LIB_ATOMIC)
                set(CMAKE_REQUIRED_LIBRARIES "${LIB_ATOMIC}")

                check_cxx_source_compiles("
                    #include <int128_t.hpp>
                    int main() {
                        primecount::maxint_t sum = 0;
                        #pragma omp parallel for reduction(+: sum)
                        for (int i = 0; i < 100; i++)
                            sum += i;
                        return 0;
                    }" OpenMP_with_libatomic)

                if(NOT OpenMP_with_libatomic)
                    set(LIB_ATOMIC "")
                endif()
            endif()
        endif()

        cmake_pop_check_state()

        if(OpenMP OR OpenMP_with_libatomic)
            if(TARGET OpenMP::OpenMP_CXX)
                set(LIB_OPENMP "OpenMP::OpenMP_CXX")
            else()
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
            endif()
        endif()
    else()
        message(STATUS "Performing Test OpenMP")
        message(STATUS "Performing Test OpenMP - Failed")
    endif()
endif()

# Check if libdivide.h works #########################################

if(WITH_LIBDIVIDE)
    message(STATUS "Performing Test libdivide")

    try_run(RUN_LIBDIVIDE COMPILE_LIBDIVIDE
            ${CMAKE_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/test/libdivide.cpp
            COMPILE_DEFINITIONS -I${CMAKE_CURRENT_SOURCE_DIR}/include)

    if(RUN_LIBDIVIDE EQUAL 0)
        message(STATUS "Performing Test libdivide - Success")
    else()
        message(STATUS "Performing Test libdivide - Failed")
    endif()
endif()

# Include S2_easy* source files ######################################

if(RUN_LIBDIVIDE EQUAL 0)
    set(LIB_SRC ${LIB_SRC} src/deleglise-rivat/S2_easy_libdivide.cpp)
else()
    set(LIB_SRC ${LIB_SRC} src/deleglise-rivat/S2_easy.cpp)
endif()

if(WITH_MPI)
    if(RUN_LIBDIVIDE EQUAL 0)
        set(LIB_SRC ${LIB_SRC} src/mpi/S2_easy_mpi_libdivide.cpp)
    else()
        set(LIB_SRC ${LIB_SRC} src/mpi/S2_easy_mpi.cpp)
    endif()
endif()

# Find POPCNT compiler flag ##########################################

if(WITH_POPCNT)
    if("${CMAKE_CXX_COMPILER}" MATCHES "icpc")
        cmake_push_check_state()
        set(CMAKE_REQUIRED_FLAGS -Werror)
        check_cxx_compiler_flag(-msse4.2 msse42)
        cmake_pop_check_state()

        if(msse42)
            set(POPCNT_FLAG "-msse4.2")
        endif()
    endif()

    if(NOT POPCNT_FLAG)
        cmake_push_check_state()
        set(CMAKE_REQUIRED_FLAGS -Werror)
        check_cxx_compiler_flag(-mpopcnt mpopcnt)
        cmake_pop_check_state()

        if(mpopcnt)
            set(POPCNT_FLAG "-mpopcnt")
        endif()
    endif()
endif()

if(NOT WITH_POPCNT)
    set(DISABLE_POPCNT "DISABLE_POPCNT")
endif()

# Testing ############################################################

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# libprimecount ######################################################

if(BUILD_SHARED_LIBS)
    add_library(libprimecount SHARED ${LIB_SRC})
    set_target_properties(libprimecount PROPERTIES OUTPUT_NAME primecount)
    set_target_properties(libprimecount PROPERTIES SOVERSION ${PRIMECOUNT_VERSION_MAJOR})
    set_target_properties(libprimecount PROPERTIES VERSION ${PRIMECOUNT_VERSION})
    target_compile_definitions(libprimecount PRIVATE "${DISABLE_POPCNT}" "${HAVE_MPI}")
    target_compile_options(libprimecount PRIVATE "${POPCNT_FLAG}")
    target_link_libraries(libprimecount PRIVATE libprimesieve "${LIB_OPENMP}" "${LIB_MPI}" "${LIB_ATOMIC}")

    target_compile_features(libprimecount
    PRIVATE
        cxx_constexpr
        cxx_lambdas
        cxx_range_for)

    target_include_directories(libprimecount PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

    install(TARGETS libprimecount
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# libprimecount-static ###############################################

if(BUILD_STATIC_LIBS)
    add_library(libprimecount-static STATIC ${LIB_SRC})
    set_target_properties(libprimecount-static PROPERTIES OUTPUT_NAME primecount)
    target_compile_definitions(libprimecount-static PRIVATE "${DISABLE_POPCNT}" "${HAVE_MPI}")
    target_compile_options(libprimecount-static PRIVATE "${POPCNT_FLAG}")
    target_link_libraries(libprimecount-static PRIVATE libprimesieve-static "${LIB_OPENMP}" "${LIB_MPI}" "${LIB_ATOMIC}")

    if(NOT BUILD_SHARED_LIBS)
        # make sure libprimecount is always defined
        add_library(libprimecount ALIAS libprimecount-static)
    endif()

    target_compile_features(libprimecount-static
    PRIVATE
        cxx_constexpr
        cxx_lambdas
        cxx_range_for)

    target_include_directories(libprimecount-static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

    install(TARGETS libprimecount-static
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# primecount binary ##################################################

if(BUILD_PRIMECOUNT)
    add_executable(primecount ${BIN_SRC})
    target_compile_definitions(primecount PRIVATE "${HAVE_MPI}")
    target_link_libraries(primecount PRIVATE libprimecount "${LIB_MPI}")
    target_compile_features(primecount PRIVATE cxx_auto_type)
    install(TARGETS primecount DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Install header #####################################################

install(FILES include/primecount.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
