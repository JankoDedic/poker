# =============================================================================
# CMake
# =============================================================================
cmake_minimum_required(VERSION 3.11)

# =============================================================================
# Project
# =============================================================================
project(Poker)

# =============================================================================
# Depedencies
# =============================================================================
add_subdirectory(third_party/span-lite)

# =============================================================================
# Library
# =============================================================================
add_library(Poker INTERFACE)
target_compile_features(Poker INTERFACE cxx_std_17)
target_include_directories(Poker INTERFACE include/)
target_link_libraries(Poker INTERFACE span-lite)

# =============================================================================
# Executable
# =============================================================================
add_executable(PokerExe main.cpp)
target_compile_features(PokerExe PRIVATE cxx_std_17)
target_link_libraries(PokerExe PRIVATE Poker)
