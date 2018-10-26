# =============================================================================
# CMake
# =============================================================================
cmake_minimum_required(VERSION 3.12)

# =============================================================================
# Project
# =============================================================================
project(poker)

# =============================================================================
# Depedencies
# =============================================================================
add_subdirectory(third_party/span-lite)

# =============================================================================
# Library
# =============================================================================
add_library(poker INTERFACE)
target_compile_features(poker INTERFACE cxx_std_17)
target_include_directories(poker INTERFACE include/)
target_link_libraries(poker INTERFACE span-lite)

# =============================================================================
# Executable
# =============================================================================
add_executable(poker-exe main.cpp)
target_compile_features(poker-exe PRIVATE cxx_std_17)
target_link_libraries(poker-exe PRIVATE poker)
