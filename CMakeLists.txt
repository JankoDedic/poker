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
add_subdirectory(third_party/span-lite EXCLUDE_FROM_ALL)
# find_path(DOCTEST_INCLUDE_DIR doctest/doctest.h)
set(DOCTEST_INCLUDE_DIR third_party/doctest)

# =============================================================================
# Library
# =============================================================================
add_library                ( poker INTERFACE                                     )
target_compile_features    ( poker INTERFACE cxx_std_17                          )
target_compile_options     ( poker INTERFACE /permissive-                        )
target_include_directories ( poker INTERFACE include      ${DOCTEST_INCLUDE_DIR} )
target_link_libraries      ( poker INTERFACE span-lite                           )

# =============================================================================
# Tests
# =============================================================================

add_executable(test
  tests/main.test.cpp
  tests/poker/hand.test.cpp
)
target_compile_features    ( test PRIVATE cxx_std_17             )
target_include_directories ( test PRIVATE ${DOCTEST_INCLUDE_DIR} )
target_link_libraries      ( test PRIVATE poker                  )

# =============================================================================
# Executable
# =============================================================================
# add_executable          ( poker-exe main.cpp             )
# target_compile_features ( poker-exe PRIVATE cxx_std_17   )
# target_compile_options  ( poker-exe PRIVATE /permissive- )
# target_link_libraries   ( poker-exe PRIVATE poker        )
