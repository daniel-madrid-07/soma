# SOMA — Arquitectura

## 1. Filosofía de ingeniería

Tres reglas duras, no negociables:

1. **Emergencia, no scripting.** Ningún movimiento se escribe. Los movimientos aparecen porque actuadores aplican fuerzas sobre huesos. Si algo se ve "correcto" pero no salió de física + mecánica, es un bug.
2. **Causalidad estricta.** Cada sistema solo conoce sus entradas físicas/dinámicas/eléctricas. El agente **no** puede leer `object.position`. Solo recibe la imagen renderizada por el cámara. La rodilla no sabe "estoy caminando"; solo recibe fuerza del cuádriceps.
3. **Unidades SI en todo.** metros, kilogramos, segundos, newtons, pascales, moles, kelvin, voltios. Sin números mágicos. Constantes de partess viven en una base de datos de parámetros, no en el código.

## 2. Patrón arquitectónico

Tres patrones combinados:

- **Capas (Layers).** L0 abajo (plataforma) → L9 arriba (tooling). Una capa solo depende de capas inferiores.
- **ECS (Entity-Component-System).** Entidad = estructura de partes (un fémur, un actuador, una nodo motora). Componente = propiedades físicas/mecánicas (masa, densidad, longitud de fibra). Sistema = simulador que opera sobre componentes. Da modularidad extrema: cada sistema es independiente y testeable en aislamiento.
- **Message bus + Blackboard.** Los sistemas no se llaman entre sí directamente. Publican señales (potencial de acción, presión de fluido, fuerza de contacto) en buses tipados. Esto respeta la causalidad: la señal es el único acoplamiento.

Regla: **un sistema nunca lee el estado interno de otro sistema.** Solo consume señales publicadas. Esto es lo que hace que todo sea "real" y no truco.

## 3. Capas (L0 → L9)

```
L9  Tooling / Dev            editor, profiler, validación vs datos reales, escenario/mundo
L8  Rendering & Visualization renderer, capas de partess toggleables, vista-cámara (alimenta visión)
L7  Agent / Cognition / AI    intención (← input usuario), planificación motora, modelo del mundo
L6  Sensors Simulation        visión, oído, vestíbulo, sensor de articulación, tacto, dolor, interocepción
L5  Control layer            nodos, capa de control, CPG, realimentacións, unión de mando, de mando externo
L4  Physiological Systems      de bombeo, de fuelle, digestivo, buffer de energía, termo, endocrino, inmune
L3  Structural / Structural       armadura, actuadores (no lineal), tirantes, tirantes, piel, grasa, props internos
L2  Physics Engine             rígidos, articulados (Featherstone), FEM/PBD, fluidos, colisión, fractura
L1  Simulation Core            ECS, scheduler multi-rate, solvers ODE/lineales, LOD manager, estructuras espaciales
L0  Platform / Foundation      matemáticas, memoria, jobs/threads, tiempo, event bus, unidades, serialización, RNG, log
```

Cross-cutting (atraviesan todo): **Units**, **LOD Manager**, **Message Bus**, **Parameter DB**, **Profiler**, **Serialization**.

## 4. Flujo de datos — el caso "caminar"

Este es el test de que la arquitectura es correcta. El usuario solo aporta **intención**.

```
[Usuario pulsa W]
      │  (única entrada: intención, no control directo)
      ▼
L7  Corteza premotora ─ genera meta: "avanzar"
      │
      ▼
L7  Corteza motora ─ traduce meta a patrón de activación deseado
      │  descending drive
      ▼
L5  Médula de patrón ─ Central Pattern Generator (CPG) genera ritmo locomotor
      │  + arcos realimentacións (estiramiento, Golgi) modulan localmente
      ▼
L5  Motonodos ─ reclutamiento (principio de tamaño) + rate coding
      │  potencial de acción (- o integrate-and-fire según LOD)
      │  viaja por axón: velocidad de conducción finita, mielina, retardo real
      ▼
L5  Unión de mando ─ convierte impulso en señal de activación [0..1]
      ▼
L3  Actuador (modelo no lineal) ─ activación → fuerza contráctil
      │  fuerza depende de longitud (F-L) y velocidad (F-V) actuales
      │  consume energía → alimenta desgaste y buffer de energía (L4)
      ▼
L3  Tendón (elástico en serie) ─ transmite fuerza, se estira, puede romperse
      ▼
L2  Hueso (cuerpo rígido) ─ recibe fuerza en punto de inserción → torque en articulación
      │  articulación respeta límites de partess (rango de movimiento)
      ▼
L2  Solver de contactos ─ pie golpea suelo → fuerza de reacción del suelo (GRF)
      ▼
L6  Husos de actuadores + Golgi + receptores articulares ─ miden longitud/tensión reales
L6  Vestíbulo (canales + otolitos) ─ mide aceleración/orientación de la cabeza
      │  aferencias sensoriales
      ▼
L7  Controlador de equilibrio ─ integra vestíbulo + sensor de articulación → corrección postural
      │  cierra el lazo (feedback), reajusta el drive descendente
      ▼
[El cuerpo avanza — porque todo lo anterior ocurrió, no porque "camine.mov" se reprodujo]
```

Si en algún punto se hace trampa (ej. el equilibrio lee la posición real del suelo en vez de la GRF + vestíbulo), la arquitectura está rota. Los tests de causalidad lo detectan.

## 5. Scheduling multi-rate

No todo corre a la misma frecuencia. El scheduler es de **paso fijo, multi-tasa**, con sub-stepping para los sistemas rígidos.

| Banda | Frecuencia | Sistemas |
|---|---|---|
| Rígida/contactos | 1000 Hz (1 ms) | rígidos, articulado, restricciones, colisión, dinámica de actuador |
| Neuronal | 1000–10000 Hz | potenciales de acción, conducción (scientific LOD) |
| Fluidos | 100–1000 Hz | fluido (lumped), aire de fuelle |
| Telemetría lenta | 1–10 Hz | buffer de energía, gestión térmica, hormonas, digestión |
| Cognición | 10–30 Hz | planificación motora, modelo del mundo, decisión |
| Visión (render-cámara) | 30–60 Hz | render offscreen que alimenta sensor |

Acoplamiento entre bandas vía el message bus con interpolación/retención (zero-order hold). Cada sistema declara su tasa; el scheduler ordena por dependencias dentro de cada tick.

Integradores: semi-implícito de Euler para rígidos estables; RK4 donde importa precisión; implícito para FEM stiff. Librería de solvers en L1, no reimplementada por sistema.

## 6. LOD — 4 niveles, por-sistema

El LOD **no** es global. Cada sistema elige su fidelidad de forma independiente, seleccionando un backend distinto tras la **misma interfaz**. Un cuerpo puede correr bomba a nivel científico y piel a tiempo real.

| Nivel | Objetivo | Ejemplos de backend |
|---|---|---|
| **Científico** | máxima precisión, offline | Actuador: micro-modelo (). Fluido: Navier-Stokes 3D (CFD). Control: - + cable eq. Tejido: FEM co-rotacional. Bomba: lumped. |
| **Simulación completa** | fiel, interactivo lento | Actuador: no lineal 3-elementos. Fluido: 1D + R–C. Control: HH simplificado. Tejido: FEM lineal. |
| **Simplificado** | rápido, aproximado | Actuador: no lineal sin elasticidad de fibra. Fluido: compartimentos lumped. Control: integrate-and-fire. Tejido: PBD (position-based dynamics). |
| **Tiempo real** | interactivo fluido | Actuador: actuador de torque con curvas F-L/F-V analíticas. Fluido: variables agregadas. Control: PD + CPG. Tejido: skinning físico ligero. |

Contrato: cambiar de nivel **no** cambia la interfaz ni rompe la causalidad. Solo cambia el coste/precisión. El `LOD Manager` puede degradar sistemas fuera de foco (ej. digestión a nivel simplificado mientras el foco es locomoción).

## 7. Stack técnico (recomendado)

Decisión de ingeniero senior. Justificada por el ecosistema científico y de rendimiento.

- **Core: C++20. [DECIDIDO]** Único con el ecosistema completo de computación científica + control fino de memoria + rendimiento. (Descartado Rust: ecosistema FEM/CFD más joven.)
- **Álgebra lineal:** Eigen.
- **ECS:** EnTT.
- **Paralelismo CPU:** oneTBB / OpenMP. **GPU:** CUDA o compute shaders (FEM, CFD, campos nodoles).
- **Físicas rígidas base (referencia/arranque):** se puede prototipar sobre Bullet/PhysX, pero el objetivo es motor propio en L2 para controlar fractura y acoplamiento actuador-hueso.
- **Scripting/experimentos:** Python vía pybind11 (configurar escenarios, correr validaciones, tunear parámetros sin recompilar).
- **Rendering:** Vulkan (o bgfx para arrancar rápido). Debug-draw propio desde el día 1.
- **Build:** CMake + vcpkg.
- **Tests:** GoogleTest (C++) + pytest (Python) para validación numérica.
- **Datos de partess:** modelos OpenSim (huesos, rutas de actuadores, brazos de momento), Visible Human Project / BodyParts3D para geometría. Constantes (densidad ósea, PCSA de actuador, volúmenes de cámara de bomba) en Parameter DB versionada.

## 8. Validación — el tip #1

Cada sistema debe poder **probarse contra la realidad**:

- Actuador no lineal → curvas fuerza-longitud y fuerza-velocidad publicadas.
- Marcha → comparar cinemática (ángulos articulares) y GRF vs datos de laboratorio de marcha (p. ej. datasets de gait).
- Bomba → curva presión-volumen (bucle PV), throughput del modelo.
- Fuelle → volúmenes secundarioes, curva de disociación de carga.
- Control → forma y velocidad del potencial de acción.

El proyecto incluye un **arnés de validación** (L9) que corre estos benchmarks y falla el build si un sistema se desvía de rangos del modelos. Sin esto, no sabemos si "funciona".

## 9. Estructura de carpetas

```
soma/
├── docs/
├── data/                  # Parameter DB, geometrías de partess, datasets de validación
├── engine/
│   ├── l0_foundation/
│   ├── l1_core/
│   ├── l2_physics/
│   ├── l3_actuators/
│   ├── l4_systems/
│   ├── l5_control/
│   ├── l6_sensors/
│   ├── l7_agent/
│   ├── l8_render/
│   └── l9_tools/
├── bindings/              # pybind11
├── scenarios/             # escenarios en Python (mundo, gravedad, objetos)
├── tests/                 # unit + validación del modelo
└── CMakeLists.txt
```

Detalle módulo a módulo en [MODULES.md](MODULES.md). Orden de construcción en [ROADMAP.md](ROADMAP.md).
