# SOMA — Simulated Organism Modeling Architecture
# Raíz del build. C++20. Se crece capa por capa (ver docs/ROADMAP.md).
cmake_minimum_required(VERSION 3.20)
project(soma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# --- L0 Foundation (header-only por ahora) ---
add_library(soma_l0_foundation INTERFACE)
target_include_directories(soma_l0_foundation INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l0_foundation)

# --- L1 Simulation Core (header-only por ahora) ---
add_library(soma_l1_core INTERFACE)
target_include_directories(soma_l1_core INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l1_core)
target_link_libraries(soma_l1_core INTERFACE soma_l0_foundation)

# --- L2 Physics Engine (header-only por ahora) ---
add_library(soma_l2_physics INTERFACE)
target_include_directories(soma_l2_physics INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l2_physics)
target_link_libraries(soma_l2_physics INTERFACE soma_l1_core)

# --- L3 Anatomy (header-only por ahora) ---
add_library(soma_l3_anatomy INTERFACE)
target_include_directories(soma_l3_anatomy INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l3_anatomy)
target_link_libraries(soma_l3_anatomy INTERFACE soma_l2_physics)

# --- L4 Physiology (header-only por ahora) ---
add_library(soma_l4_physiology INTERFACE)
target_include_directories(soma_l4_physiology INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l4_physiology)
target_link_libraries(soma_l4_physiology INTERFACE soma_l0_foundation)

# --- L5 Nervous / L6 Sensory / L7 Brain (header-only por ahora) ---
add_library(soma_l5_nervous INTERFACE)
target_include_directories(soma_l5_nervous INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l5_nervous)
target_link_libraries(soma_l5_nervous INTERFACE soma_l0_foundation)

add_library(soma_l6_sensory INTERFACE)
target_include_directories(soma_l6_sensory INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l6_sensory)
target_link_libraries(soma_l6_sensory INTERFACE soma_l2_physics)

add_library(soma_l7_brain INTERFACE)
target_include_directories(soma_l7_brain INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l7_brain)
target_link_libraries(soma_l7_brain INTERFACE soma_l0_foundation)

# --- L9 Tools (validación, benchmarks) ---
add_library(soma_l9_tools INTERFACE)
target_include_directories(soma_l9_tools INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l9_tools)
target_link_libraries(soma_l9_tools INTERFACE soma_l0_foundation)

# --- Tests ---
enable_testing()
add_subdirectory(tests)
