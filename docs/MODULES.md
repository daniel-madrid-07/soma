# SOMA — Mapa de módulos

Cientos de módulos, agrupados por capa. Cada módulo es independiente, testeable en aislamiento, con una interfaz pública mínima. Un módulo solo depende de módulos de su capa o inferiores.

Notación: `└─ modulo` = módulo hoja (unidad implementable). El orden dentro de cada capa es aproximadamente el orden de dependencia.

---

## L0 — Foundation

```
core/math
  ├─ scalar            (tipos, epsilon, clamp, lerp)
  ├─ vec               (vec2/3/4)
  ├─ mat               (mat3/4, mat NxM)
  ├─ quat              (rotaciones)
  ├─ transform         (SE3, poses)
  ├─ tensor            (campos, para FEM/CFD)
  ├─ spline            (rutas de actuadores, superficies de wrapping)
  ├─ interp            (interpolación, zero-order hold entre bandas)
  └─ random            (RNG determinista, distribuciones)
core/units             (SI, dimensional analysis, chequeo de unidades en compilación)
core/memory
  ├─ allocators        (pool, arena, stack)
  ├─ handles           (referencias estables a entidades)
  └─ soa               (structure-of-arrays para datos calientes)
core/containers        (sparse set, ring buffer, small vector)
core/jobs
  ├─ thread_pool
  ├─ task_graph        (dependencias entre tareas)
  └─ parallel_for
core/time
  ├─ clock
  ├─ fixed_step
  └─ rate_group        (bandas de frecuencia)
core/events
  ├─ message_bus       (buses tipados)
  ├─ signal            (pub/sub)
  └─ blackboard        (estado compartido tipado)
core/serialization     (guardar/cargar estado, snapshots deterministas)
core/assets            (carga de geometría, Parameter DB)
core/config            (parámetros, overrides por escenario)
core/log               (logging estructurado)
core/profile           (timers, contadores, marcadores)
core/assert            (invariantes, chequeos de causalidad en debug)
```

---

## L1 — Simulation Core

```
sim/ecs
  ├─ entity
  ├─ component_store    (SoA)
  ├─ system_registry
  ├─ query
  └─ archetype
sim/scheduler
  ├─ rate_scheduler     (multi-rate, paso fijo)
  ├─ substep
  ├─ dependency_sort    (orden de sistemas por dependencias)
  └─ coupler            (traspaso entre bandas con interpolación)
sim/solvers
  ├─ ode                (Euler semi-implícito, RK4, implícito)
  ├─ linear             (CG, LU esparcido, precondicionadores)
  ├─ newton             (no lineal, para FEM)
  ├─ constraint         (proyección, LCP/PGS)
  └─ integrator_config  (selección por sistema)
sim/spatial
  ├─ aabb
  ├─ bvh
  ├─ octree
  ├─ uniform_grid       (vecindad para SPH)
  └─ broadphase
sim/lod
  ├─ lod_manager        (nivel por sistema)
  ├─ lod_policy         (degradación por foco/coste)
  └─ backend_selector   (misma interfaz, distinto backend)
sim/state
  ├─ snapshot           (guardar/restaurar cuerpo completo)
  ├─ recorder           (grabar señales para análisis — NO para replay animado)
  └─ determinism        (semillas, orden estable)
```

---

## L2 — Physics Engine

```
physics/rigid
  ├─ body               (masa, inercia, densidad)
  ├─ integrator
  └─ sleeping
physics/articulated
  ├─ featherstone       (dinámica en coordenadas reducidas para el armadura)
  ├─ joint_dof
  └─ forward_dynamics
physics/constraints
  ├─ joint_constraint   (esférica, bisagra, silla, pivote)
  ├─ limit_constraint   (rango de movimiento de partes)
  ├─ contact_constraint
  └─ friction
physics/collision
  ├─ shapes             (cápsula, malla convexa, SDF)
  ├─ narrowphase        (GJK/EPA, SDF)
  ├─ contact_manifold
  └─ ccd                (continuous collision para impactos rápidos)
physics/softbody
  ├─ fem_linear
  ├─ fem_corotational   (grandes deformaciones)
  ├─ fem_hyperelastic   (tejido mecánico, Neo-Hookean/Ogden)
  ├─ pbd                (position-based dynamics, LOD tiempo real)
  ├─ xpbd
  ├─ mass_spring        (LOD simplificado)
  └─ contact_coupling   (soft ↔ rígido)
physics/fluids
  ├─ sph                (partículas, fluido/aire aproximado)
  ├─ lumped             (compartimentos, R–C)
  ├─ cfd_1d             (redes de tubos 1D)
  ├─ cfd_3d             (Navier-Stokes, scientific)
  └─ diffusion          (gases, calor)
physics/damage
  ├─ stress_field       (tensor de tensiones en huesos/tejido)
  ├─ fracture           (fractura ósea por umbral de tensión)
  ├─ fissure            (fisura, daño parcial)
  ├─ tear               (desgarro en soft body / tirante / tirante)
  └─ fatigue_damage     (acumulación por ciclos)
physics/forces
  ├─ gravity
  ├─ drag
  └─ external           (empujones del escenario, para tests de equilibrio)
```

---

## L3 — Structural / Structural

```
parts/skeleton
  ├─ bone               (entidad rígida: densidad, peso, geometría)
  ├─ bone_db            (los ~206 huesos reales desde Parameter DB)
  ├─ joint              (definición articular, DOF, cartílago)
  ├─ joint_limits       (rango de movimiento por articulación)
  ├─ ossification       (opcional: crecimiento/edad)
  └─ skeleton_assembly  (grafo completo hueso-articulación)
parts/actuator
  ├─ spring_actuator        (contráctil + elástico serie + elástico paralelo)
  ├─ crossbridge        (, scientific LOD)
  ├─ torque_actuator    (LOD tiempo real)
  ├─ fiber              (tipos I/II, PCSA, longitud óptima)
  ├─ activation         (dinámica de activación desde señal nodol)
  ├─ force_length       (curva F-L)
  ├─ force_velocity     (curva F-V)
  ├─ actuator_path        (origen, inserción, vía puntos)
  ├─ wrapping           (actuador envuelve hueso/superficie)
  ├─ moment_arm         (brazo de momento dinámico)
  ├─ fatigue            (desgaste por consumo)
  ├─ energy_cost        (energía por contracción → L4)
  └─ actuator_db          (los actuadores reales)
parts/tendon
  ├─ tendon             (elástico en serie, transmite solo fuerza)
  ├─ tendon_rupture     (rotura por sobrecarga)
  └─ tendon_db
parts/ligament
  ├─ ligament           (estabiliza articulación)
  ├─ sprain             (esguince, lesión parcial)
  ├─ ligament_tear
  └─ ligament_db
parts/binder
  ├─ fascia
  └─ cartilage
parts/skin
  ├─ skin_membrane      (FEM membrana sobre el cuerpo)
  ├─ stretch_compress
  ├─ impact_response
  ├─ layers             (epidermis/dermis)
  └─ wounds             (cortes, laceraciones → sangrado L4)
parts/relleno
  ├─ fat_layer          (soft body subcutáneo)
  ├─ distribution       (peso, distribución por zona)
  └─ compression
parts/props
  ├─ prop_body         (prop interno como cuerpo físico deformable)
  ├─ placement          (posición de partes, sujeción)
  └─ prop_db
```

---

## L4 — Physiological Systems

```
physio/de bombeo
  ├─ pump
  │   ├─ chambers        (2 depósitos, 2 cámaras como cámaras físicas)
  │   ├─ valves          (de entrada, tricúspide, de salida, secundario)
  │   ├─ electrophysiology (nodo SA/AV, His-Purkinje; FitzHugh-Nagumo→ por LOD)
  │   ├─ contraction     (contracción activa del cámara → impulsa fluido)
  │   ├─ pv_loop         (bucle presión-volumen, validación)
  │   ├─ autonomic_input (responde a mando alto/paramando alto de L5)
  │   ├─ fatigue
  │   └─ pathology       (arritmia, insuficiencia)
  ├─ vessels
  │   ├─ out_tubes        (con distensibilidad, R–C)
  │   ├─ return_tubes           (retorno, capacitancia)
  │   ├─ capillaries     (intercambio con tejido)
  │   └─ de tubos_graph  (red completa del cuerpo)
  └─ fluid
      ├─ volume
      ├─ pressure
      ├─ velocity
      ├─ oxygenation     (carga unido a hemoglobina, curva de disociación)
      ├─ co2
      ├─ ph
      ├─ temperature     (transporta calor → termo)
      └─ hemostasis      (coagulación en heridas)
physio/bellows
  ├─ airways            (tráquea, bronquios)
  ├─ lungs              (volumen, distensibilidad)
  ├─ air_cells            (intercambio gaseoso carga/residuo)
  ├─ breathing_drive    (diafragma + intercostales, actuadores reales de L3)
  ├─ transfer       (difusión celda↔tubo fino)
  ├─ lung_capacity      (volúmenes: tidal, vital, residual)
  └─ gas_transport      (carga/residuo en fluido ↔ tejidos)
physio/digestive
  ├─ mouth              (masticación, saliva)
  ├─ esophagus          (peristalsis)
  ├─ stomach            (mezcla, ácido)
  ├─ intestines         (peristalsis, tránsito)
  ├─ absorption         (nutrientes → fluido)
  └─ nutrients          (glucosa, grasas, proteínas → buffer de energía)
physio/energy
  ├─ energy                (moneda energética)
  ├─ pcr                (reserva rápida, sprint)
  ├─ glycolysis         (ansostenible)
  ├─ oxidative          (sostenible, usa carga)
  ├─ glycogen_store
  ├─ fat_store
  ├─ energy_balance     (throughput vs ingesta)
  └─ fatigue_global     (cansancio como consecuencia energética)
physio/thermoregulation
  ├─ heat_production    (buffer de energía + trabajo de actuador)
  ├─ conduction
  ├─ convection
  ├─ radiation
  ├─ evaporation
  ├─ sweat              (glándulas, pérdida de agua)
  └─ shivering          (genera calor vía actuador)
physio/endocrine
  ├─ hormone            (modelo genérico: secreción, vida media, receptor)
  ├─ adrenaline         (modula bomba, actuador)
  ├─ cortisol
  ├─ insulin_glucagon   (regula glucosa)
  └─ hormone_bus        (modulación difusa de otros sistemas)
physio/immune
  ├─ inflammation
  ├─ infection
  ├─ healing            (cicatrización de heridas/fracturas)
  └─ immune_response
physio/renal
  ├─ fluid_balance
  └─ electrolytes
physio/homeostasis
  ├─ setpoints          (valores objetivo)
  └─ regulator          (integra reguladores: pH, temp, glucosa, presión)
```

---

## L5 — Control layer

```
control/neuron
  ├─ node_model     (potencial de acción, scientific)
  ├─ integrate_fire     (LOD rápido)
  ├─ cable_equation     (propagación a lo largo del axón)
  ├─ myelin             (conducción saltatoria, velocidad)
  ├─ synapse            (dinámica, retardo)
  └─ refractory
control/cns
  ├─ spinal_cord        (sustrato de realimentacións + CPG)
  ├─ tracts             (vías ascendentes/descendentes)
  └─ brain_interface    (puerta a L7)
control/pattern
  ├─ reflex_stretch     (arco realimentación miotático)
  ├─ reflex_golgi       (inhibición por tensión)
  ├─ reflex_withdrawal  (retirada por dolor)
  ├─ cpg                (central pattern generator, ritmo locomotor)
  └─ interneurons
control/motor
  ├─ motoneuron
  ├─ motor_unit         (motonodo + fibras que inerva)
  ├─ recruitment        (principio de tamaño)
  ├─ rate_coding
  └─ nmj                (unión de mando → activación de actuador L3)
control/afferent
  ├─ spindle            (sensor de actuador: longitud/velocidad)
  ├─ golgi_organ        (prop interno tendinoso de Golgi: tensión)
  ├─ joint_receptor
  └─ nociceptor         (dolor → L6)
control/autonomic
  ├─ sympathetic        (lucha/huida → bomba, tubos)
  ├─ parasympathetic    (reposo)
  └─ visceral_control   (regula props internos de L4)
control/conduction
  ├─ nerve_bundle       (controles reales, haces)
  ├─ conduction_delay   (retardo real por distancia/velocidad)
  └─ nerve_db
```

---

## L6 — Sensors Simulation

```
sensory/vision
  ├─ eye_optics         (córnea, cristalino, apertura de pupila)
  ├─ accommodation      (enfoque)
  ├─ sensor             (fotorreceptores, conos/bastones)
  ├─ eye_render         (render offscreen desde el cámara — única fuente de info visual)
  ├─ visual_field       (campo de visión, agudeza periférica vs foveal)
  ├─ optic_signal       (imagen → señal a L7; el agente NO recibe posiciones)
  └─ eye_movement       (sacadas, seguimiento, vergencia — via actuadores de cámara L3)
sensory/hearing
  ├─ sound_propagation  (distancia, atenuación, oclusión)
  ├─ hrtf               (dirección por diferencias entre oídos)
  ├─ cochlea            (frecuencia → señal neural)
  └─ auditory_signal    (→ L7)
sensory/IMU
  ├─ semicircular       (aceleración angular de la cabeza)
  ├─ otolith            (aceleración lineal + gravedad)
  └─ IMU_signal  (→ equilibrio L7)
sensory/proprioception
  ├─ from_spindles      (longitud de actuador)
  ├─ from_golgi         (tensión)
  ├─ joint_angle_sense
  └─ body_schema        (mapa sensor → L7)
sensory/somatic
  ├─ mechanoreceptor    (tacto, presión)
  ├─ thermoreceptor     (temperatura de piel)
  └─ pain
      ├─ nociception    (señal desde daño real de L2/L3)
      ├─ pain_pathway   (viaja por controles reales → L7)
      └─ pain_behavior  (modula comportamiento en L7)
sensory/interoception
  ├─ hunger             (desde buffer de energía L4)
  ├─ air_hunger         (desde residuo/carga L4)
  ├─ fatigue_sense
  └─ thirst
```

---

## L7 — Agent / Cognition / AI

```
agent/intention
  ├─ intention_api      (ÚNICA entrada del usuario: metas, no controles)
  ├─ goal_stack         (avanzar, mirar, agarrar…)
  └─ input_mapper       (W → intención "avanzar", nunca → torque)
agent/perception
  ├─ visual_processing  (interpreta la imagen del cámara: bordes, objetos, profundidad)
  ├─ auditory_processing
  ├─ world_model        (modelo interno del entorno construido SOLO desde sentidos)
  └─ attention
agent/motor
  ├─ motor_cortex       (traduce meta a patrón de activación deseado)
  ├─ premotor           (planificación de secuencias)
  ├─ cerebellum         (coordinación fina, corrección predictiva)
  ├─ basal_ganglia      (selección de acción)
  └─ descending_drive   (comando hacia capa de control L5)
agent/balance
  ├─ postural_control   (integra vestíbulo + sensor de articulación)
  ├─ com_estimation     (estima centro de masa desde sentidos)
  ├─ balance_controller (correcciones continuas → drive)
  └─ fall_recovery
agent/cognition
  ├─ decision           (comportamiento de alto nivel)
  ├─ memory             (opcional)
  └─ motor_learning     (opcional: mejora con la práctica)
```

**Nota crítica:** `agent/*` solo consume señales sensoriales (L6) y publica drive motor (→ L5). No accede a estado físico del mundo ni del propio cuerpo salvo por vía sensorial. Esto se verifica con tests de causalidad.

---

## L8 — Rendering & Visualization

```
render/core
  ├─ device             (Vulkan/bgfx)
  ├─ pipeline
  ├─ mesh
  └─ material
render/parts_layers
  ├─ skin_layer
  ├─ actuator_layer
  ├─ skeleton_layer
  ├─ prop_layer
  ├─ de tubos_layer     (flujo de fluido visible)
  └─ neural_layer       (señales de controls visibles)
render/debug
  ├─ vectors            (fuerzas, torques)
  ├─ activation_heatmap (activación de actuador)
  ├─ contact_points
  └─ com_marker
render/eye_target       (render offscreen que alimenta L6 visión)
render/camera
```

---

## L9 — Tooling / Dev

```
tools/editor
  ├─ inspector          (ver/editar componentes de cualquier entidad)
  ├─ time_control       (pausa, paso a paso, cámara lenta)
  └─ scenario_editor
tools/world
  ├─ ground
  ├─ gravity_config
  ├─ objects            (obstáculos, objetos manipulables)
  └─ environment        (temperatura ambiente, sonido)
tools/validation
  ├─ harness            (corre benchmarks del modelos)
  ├─ gait_benchmark     (cinemática + GRF vs datos reales)
  ├─ actuator_benchmark   (curvas F-L / F-V)
  ├─ pump_benchmark   (bucle PV, throughput)
  └─ causality_check    (asegura que ningún sistema hace trampa)
tools/profiler
tools/recorder_ui       (inspeccionar señales grabadas)
```

---

## Recuento

~L0: 40 · L1: 25 · L2: 40 · L3: 55 · L4: 70 · L5: 35 · L6: 35 · L7: 30 · L8: 20 · L9: 20 → **~370 módulos hoja.**

No se implementan todos a la vez. Orden en [ROADMAP.md](ROADMAP.md).
