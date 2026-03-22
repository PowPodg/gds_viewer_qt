cmake_minimum_required(VERSION 3.20)

project(GdsWorkspace VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" OFF)

# For single-config generators (e.g. Ninja, Unix Makefiles), default to RelWithDebInfo.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

option(ENABLE_IPO "Enable interprocedural optimization (LTO/IPO) in optimized builds" ON)
option(ENABLE_NATIVE_OPT "Enable CPU-specific optimizations for local machine" OFF)

include(CheckIPOSupported)

if(ENABLE_IPO)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
    else()
        message(STATUS "IPO/LTO not supported: ${IPO_ERROR}")
    endif()
endif()

if(MSVC)
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Ob2>
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/DNDEBUG>

        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/Ob2>
        $<$<CONFIG:RelWithDebInfo>:/Oi>
        $<$<CONFIG:RelWithDebInfo>:/Gy>
        $<$<CONFIG:RelWithDebInfo>:/DNDEBUG>

        $<$<CONFIG:MinSizeRel>:/O1>
        $<$<CONFIG:MinSizeRel>:/Ob1>
        $<$<CONFIG:MinSizeRel>:/Oi>
        $<$<CONFIG:MinSizeRel>:/Gy>
        $<$<CONFIG:MinSizeRel>:/DNDEBUG>
    )

    add_link_options(
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:REF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/OPT:REF>
        $<$<CONFIG:MinSizeRel>:/OPT:ICF>
    )

    if(ENABLE_NATIVE_OPT)
        add_compile_options(
            $<$<CONFIG:Release>:/arch:AVX2>
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
        )
    endif()
else()
    add_compile_options(
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>

        $<$<CONFIG:RelWithDebInfo>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-DNDEBUG>

        $<$<CONFIG:MinSizeRel>:-Os>
        $<$<CONFIG:MinSizeRel>:-DNDEBUG>
    )

    if(ENABLE_NATIVE_OPT)
        add_compile_options(
            $<$<CONFIG:Release>:-march=native>
            $<$<CONFIG:Release>:-mtune=native>
            $<$<CONFIG:RelWithDebInfo>:-march=native>
            $<$<CONFIG:RelWithDebInfo>:-mtune=native>
        )
    endif()
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(gds_core)
add_subdirectory(gds_viewer)
