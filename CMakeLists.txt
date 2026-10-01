cmake_minimum_required(VERSION 3.14)
project(prediction_market_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimized build if none is specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Header-only library: rng.hpp, arbitrage.hpp, market_maker.hpp, simulator.hpp
add_library(engine_headers INTERFACE)
target_include_directories(engine_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Warnings (kept off -ffast-math on purpose: it would break floating-point parity with Python)
function(enable_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# Single-path run (port of main.py)
add_executable(engine main.cpp)
target_link_libraries(engine PRIVATE engine_headers)
enable_warnings(engine)

# Monte Carlo harness (port of the Monte Carlo script)
add_executable(mc monte_carlo.cpp)
target_link_libraries(mc PRIVATE engine_headers)
enable_warnings(mc)