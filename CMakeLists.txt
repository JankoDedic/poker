# =============================================================================
# CMake
# =============================================================================
cmake_minimum_required(VERSION 3.12)

# =============================================================================
# Project
# =============================================================================
project(poker)

# =============================================================================
# Dependencies
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
# Tests
# =============================================================================
find_path(DOCTEST_INCLUDE_DIR doctest/doctest.h)

add_executable(poker-tests
  tests/main.test.cpp
)
target_include_directories(poker-tests
  PRIVATE
    ${DOCTEST_INCLUDE_DIR}
)
target_link_libraries(poker-tests
  PRIVATE
    poker
)

# =============================================================================
# Executable
# =============================================================================
add_executable(poker-exe main.cpp)
target_compile_features(poker-exe PRIVATE cxx_std_17)
target_link_libraries(poker-exe PRIVATE poker)
