# =============================================================================
# CMake
# =============================================================================
cmake_minimum_required(VERSION 3.9.2 FATAL_ERROR)

# =============================================================================
# Project
# =============================================================================
project(poker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

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
target_include_directories ( poker-tests PRIVATE ${DOCTEST_INCLUDE_DIR} )
target_link_libraries      ( poker-tests PRIVATE poker                  )
