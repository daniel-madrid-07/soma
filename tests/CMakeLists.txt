# SOMA — tests. Cada slice de cada fase deja aquí una prueba verificable.
# De momento pruebas self-contained (assert + static_assert), sin dependencias
# externas, para que el build arranque sin vcpkg. GoogleTest se integra al madurar L1.

function(soma_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE
    soma_l0_foundation soma_l1_core soma_l2_physics soma_l3_anatomy
    soma_l4_physiology soma_l5_nervous soma_l6_sensory soma_l7_brain soma_l9_tools)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})  # support/
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# --- Fase 0 ---
soma_test(test_units)          # unidades SI + chequeo dimensional
soma_test(test_math)           # vec/mat/quat/transform
soma_test(test_foundation)     # random/time/events/log/assert
soma_test(test_phase0)         # integración: ECS + bus + scheduler multi-tasa + solver
# --- Fase 1 ---
soma_test(test_phase1_rigid)   # cuerpo rígido: caída, giro, reposo en el suelo
soma_test(test_phase1_joint)   # articulación esférica: péndulo de un hueso
soma_test(test_phase1_ragdoll) # esqueleto articulado: cadena de 3 huesos
soma_test(test_phase1_limit)   # límite articular anatómico (rango de movimiento)
# --- Fase 2 ---
soma_test(test_phase2_muscle)  # músculo Hill mueve el hueso (activación -> flexión)
# --- Fase 3 ---
soma_test(test_phase3_posture)   # control motor: antagonistas mantienen postura + rechazo de perturbación
soma_test(test_phase3_cpg)       # CPG (Matsuoka): ritmo estable en antifase
soma_test(test_phase3_cpg_joint) # CPG conduce una articulación: movimiento rítmico emergente
soma_test(test_phase3_coupling)  # osciladores acoplados: bloqueo a desfase deseado
soma_test(test_phase3_leg)       # pierna 2 segmentos: coordinación cadera-rodilla por CPG
# --- Fase 5 (equilibrio) ---
soma_test(test_phase5_balance)   # péndulo invertido erguido por vestíbulo + músculos
soma_test(test_phase5_stand)     # de pie sin pin ni rig, sobre un pie físico (talón+punta)
# --- Fase 6 (locomoción) ---
soma_test(test_phase6_locomotion) # intención → CPG → músculos → suelo → avanza
soma_test(test_gait_benchmark)    # métricas de marcha validadas (velocidad, cadencia, GRF)
# --- Fase 7 (visión) ---
soma_test(test_phase7_vision)     # el cerebro percibe dirección solo desde la retina
soma_test(test_phase7_navigate)   # camina hacia lo que ve (servovisión + locomoción)
# --- Fase 8 (fisiología energética) ---
soma_test(test_phase8_heart)      # corazón: presión, gasto y FE fisiológicos
soma_test(test_phase8_energy)     # metabolismo+respiración: fatiga como consecuencia del consumo
