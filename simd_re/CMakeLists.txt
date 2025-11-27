cmake_minimum_required(VERSION 3.14)
project(simd_bench C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

######################################################################
# Google Benchmark
######################################################################
find_package(benchmark REQUIRED)

######################################################################
# Architecture detection
######################################################################

set(SIMD_SOURCES "")

# x86-64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|x64")
    message(STATUS "Building for x86_64 (SSE/AVX/AVX512)")
    list(APPEND SIMD_SOURCES x86.S)
endif()

# ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    message(STATUS "Building for AArch64 (NEON/SVE)")
    list(APPEND SIMD_SOURCES arm64.S)
endif()

if(NOT SIMD_SOURCES)
    message(FATAL_ERROR "Unknown architecture: cannot select SIMD assembly source.")
endif()

######################################################################
# Build target
######################################################################

add_executable(simd_bench
    bench.cpp
    ${SIMD_SOURCES}
)

######################################################################
# Compiler flags for C++
######################################################################
target_compile_options(simd_bench PRIVATE -march=native)

######################################################################
# Link Google Benchmark
######################################################################
target_link_libraries(simd_bench PRIVATE benchmark::benchmark pthread)
