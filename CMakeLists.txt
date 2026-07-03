# SOMA — Simulated Object Motion Architecture
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

# --- L3 Actuators (header-only por ahora) ---
add_library(soma_l3_actuators INTERFACE)
target_include_directories(soma_l3_actuators INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l3_actuators)
target_link_libraries(soma_l3_actuators INTERFACE soma_l2_physics)

# --- L4 Physiology (header-only por ahora) ---
add_library(soma_l4_systems INTERFACE)
target_include_directories(soma_l4_systems INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l4_systems)
target_link_libraries(soma_l4_systems INTERFACE soma_l0_foundation)

# --- L5 Control / L6 Sensors / L7 Agent (header-only por ahora) ---
add_library(soma_l5_control INTERFACE)
target_include_directories(soma_l5_control INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l5_control)
target_link_libraries(soma_l5_control INTERFACE soma_l0_foundation)

add_library(soma_l6_sensors INTERFACE)
target_include_directories(soma_l6_sensors INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l6_sensors)
target_link_libraries(soma_l6_sensors INTERFACE soma_l2_physics)

add_library(soma_l7_agent INTERFACE)
target_include_directories(soma_l7_agent INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l7_agent)
target_link_libraries(soma_l7_agent INTERFACE soma_l0_foundation)

# --- L9 Tools (validación, benchmarks) ---
add_library(soma_l9_tools INTERFACE)
target_include_directories(soma_l9_tools INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/l9_tools)
target_link_libraries(soma_l9_tools INTERFACE soma_l0_foundation)

# --- Tests ---
enable_testing()
add_subdirectory(tests)
