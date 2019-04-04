# =============================================================================
# CMake
# =============================================================================
cmake_minimum_required(VERSION 3.9.2)

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
target_include_directories ( poker INTERFACE include      ${DOCTEST_INCLUDE_DIR} )
target_link_libraries      ( poker INTERFACE span-lite                           )

if(MSVC)
  target_compile_options   ( poker INTERFACE /permissive-                        )
endif()

# =============================================================================
# Tests
# =============================================================================

add_executable(
  poker-tests
    tests/main.test.cpp
    tests/poker/hand.test.cpp
    tests/poker/detail/round.test.cpp
    tests/poker/detail/betting_round.test.cpp
    tests/poker/dealer.test.cpp
    tests/poker/table.test.cpp
)
target_compile_features    ( poker-tests PRIVATE cxx_std_17             )
target_include_directories ( poker-tests PRIVATE ${DOCTEST_INCLUDE_DIR} )
target_link_libraries      ( poker-tests PRIVATE poker                  )

# =============================================================================
# Executable
# =============================================================================
# add_executable          ( poker-exe main.cpp             )
# target_compile_features ( poker-exe PRIVATE cxx_std_17   )
# target_compile_options  ( poker-exe PRIVATE /permissive- )
# target_link_libraries   ( poker-exe PRIVATE poker        )
