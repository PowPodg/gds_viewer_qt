cmake_minimum_required(VERSION 3.20)
project(gds_core LANGUAGES CXX)

add_library(gds_core
    src/GdsReader.cpp
    src/GdsLibrary.cpp
    src/GdsStructure.cpp
    src/GdsElements.cpp
    src/internal/InputStream.cpp
    src/internal/Record.cpp
    src/internal/RecordReader.cpp
    src/internal/Parser.cpp
)

target_include_directories(gds_core PUBLIC include)

target_compile_features(gds_core PUBLIC cxx_std_20)

target_compile_options(gds_core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
