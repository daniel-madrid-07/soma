# SOMA — Arquitectura

## 1. Filosofía de ingeniería

Tres reglas duras, no negociables:

1. **Emergencia, no scripting.** Ningún movimiento se escribe. Los movimientos aparecen porque músculos aplican fuerzas sobre huesos. Si algo se ve "correcto" pero no salió de física + biología, es un bug.
2. **Causalidad estricta.** Cada sistema solo conoce sus entradas físicas/químicas/eléctricas. El cerebro **no** puede leer `object.position`. Solo recibe la imagen renderizada por el ojo. La rodilla no sabe "estoy caminando"; solo recibe fuerza del cuádriceps.
3. **Unidades SI en todo.** metros, kilogramos, segundos, newtons, pascales, moles, kelvin, voltios. Sin números mágicos. Constantes anatómicas viven en una base de datos de parámetros, no en el código.

## 2. Patrón arquitectónico

Tres patrones combinados:

- **Capas (Layers).** L0 abajo (plataforma) → L9 arriba (tooling). Una capa solo depende de capas inferiores.
- **ECS (Entity-Component-System).** Entidad = estructura anatómica (un fémur, un músculo, una neurona motora). Componente = propiedades físicas/biológicas (masa, densidad, longitud de fibra). Sistema = simulador que opera sobre componentes. Da modularidad extrema: cada sistema es independiente y testeable en aislamiento.
- **Message bus + Blackboard.** Los sistemas no se llaman entre sí directamente. Publican señales (potencial de acción, presión sanguínea, fuerza de contacto) en buses tipados. Esto respeta la causalidad: la señal es el único acoplamiento.

Regla: **un sistema nunca lee el estado interno de otro sistema.** Solo consume señales publicadas. Esto es lo que hace que todo sea "real" y no truco.

## 3. Capas (L0 → L9)

```
L9  Tooling / Dev            editor, profiler, validación vs datos reales, escenario/mundo
L8  Rendering & Visualization renderer, capas anatómicas toggleables, vista-ojo (alimenta visión)
L7  Brain / Cognition / AI    intención (← input usuario), planificación motora, modelo del mundo
L6  Sensory Simulation        visión, oído, vestíbulo, propiocepción, tacto, dolor, interocepción
L5  Nervous System            neuronas, médula, CPG, reflejos, unión neuromuscular, autónomo
L4  Physiological Systems      cardiovascular, respiratorio, digestivo, metabolismo, termo, endocrino, inmune
L3  Anatomy / Structural       esqueleto, músculos (Hill), tendones, ligamentos, piel, grasa, órganos
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
L5  Médula espinal ─ Central Pattern Generator (CPG) genera ritmo locomotor
      │  + arcos reflejos (estiramiento, Golgi) modulan localmente
      ▼
L5  Motoneuronas ─ reclutamiento (principio de tamaño) + rate coding
      │  potencial de acción (Hodgkin-Huxley o integrate-and-fire según LOD)
      │  viaja por axón: velocidad de conducción finita, mielina, retardo real
      ▼
L5  Unión neuromuscular ─ convierte impulso en señal de activación [0..1]
      ▼
L3  Músculo (modelo Hill) ─ activación → fuerza contráctil
      │  fuerza depende de longitud (F-L) y velocidad (F-V) actuales
      │  consume ATP → alimenta fatiga y metabolismo (L4)
      ▼
L3  Tendón (elástico en serie) ─ transmite fuerza, se estira, puede romperse
      ▼
L2  Hueso (cuerpo rígido) ─ recibe fuerza en punto de inserción → torque en articulación
      │  articulación respeta límites anatómicos (rango de movimiento)
      ▼
L2  Solver de contactos ─ pie golpea suelo → fuerza de reacción del suelo (GRF)
      ▼
L6  Husos musculares + Golgi + receptores articulares ─ miden longitud/tensión reales
L6  Vestíbulo (canales + otolitos) ─ mide aceleración/orientación de la cabeza
      │  aferencias sensoriales
      ▼
L7  Controlador de equilibrio ─ integra vestíbulo + propiocepción → corrección postural
      │  cierra el lazo (feedback), reajusta el drive descendente
      ▼
[El cuerpo avanza — porque todo lo anterior ocurrió, no porque "camine.mov" se reprodujo]
```

Si en algún punto se hace trampa (ej. el equilibrio lee la posición real del suelo en vez de la GRF + vestíbulo), la arquitectura está rota. Los tests de causalidad lo detectan.

## 5. Scheduling multi-rate

No todo corre a la misma frecuencia. El scheduler es de **paso fijo, multi-tasa**, con sub-stepping para los sistemas rígidos.

| Banda | Frecuencia | Sistemas |
|---|---|---|
| Rígida/contactos | 1000 Hz (1 ms) | rígidos, articulado, restricciones, colisión, dinámica muscular |
| Neuronal | 1000–10000 Hz | potenciales de acción, conducción (scientific LOD) |
| Fluidos | 100–1000 Hz | sangre (lumped), aire respiratorio |
| Fisiología lenta | 1–10 Hz | metabolismo, termorregulación, hormonas, digestión |
| Cognición | 10–30 Hz | planificación motora, modelo del mundo, decisión |
| Visión (render-ojo) | 30–60 Hz | render offscreen que alimenta retina |

Acoplamiento entre bandas vía el message bus con interpolación/retención (zero-order hold). Cada sistema declara su tasa; el scheduler ordena por dependencias dentro de cada tick.

Integradores: semi-implícito de Euler para rígidos estables; RK4 donde importa precisión; implícito para FEM stiff. Librería de solvers en L1, no reimplementada por sistema.

## 6. LOD — 4 niveles, por-sistema

El LOD **no** es global. Cada sistema elige su fidelidad de forma independiente, seleccionando un backend distinto tras la **misma interfaz**. Un cuerpo puede correr corazón a nivel científico y piel a tiempo real.

| Nivel | Objetivo | Ejemplos de backend |
|---|---|---|
| **Científico** | máxima precisión, offline | Músculo: cross-bridge (Huxley). Sangre: Navier-Stokes 3D (CFD). Nervio: Hodgkin-Huxley + cable eq. Tejido: FEM co-rotacional. Corazón: electrofisiología bidominio. |
| **Simulación completa** | fiel, interactivo lento | Músculo: Hill 3-elementos. Sangre: 1D + Windkessel. Nervio: HH simplificado. Tejido: FEM lineal. |
| **Simplificado** | rápido, aproximado | Músculo: Hill sin elasticidad de fibra. Sangre: compartimentos lumped. Nervio: integrate-and-fire. Tejido: PBD (position-based dynamics). |
| **Tiempo real** | interactivo fluido | Músculo: actuador de torque con curvas F-L/F-V analíticas. Sangre: variables agregadas. Control: PD + CPG. Tejido: skinning físico ligero. |

Contrato: cambiar de nivel **no** cambia la interfaz ni rompe la causalidad. Solo cambia el coste/precisión. El `LOD Manager` puede degradar sistemas fuera de foco (ej. digestión a nivel simplificado mientras el foco es locomoción).

## 7. Stack técnico (recomendado)

Decisión de ingeniero senior. Justificada por el ecosistema científico y de rendimiento.

- **Core: C++20. [DECIDIDO]** Único con el ecosistema completo de computación científica + control fino de memoria + rendimiento. (Descartado Rust: ecosistema FEM/CFD más joven.)
- **Álgebra lineal:** Eigen.
- **ECS:** EnTT.
- **Paralelismo CPU:** oneTBB / OpenMP. **GPU:** CUDA o compute shaders (FEM, CFD, campos neuronales).
- **Físicas rígidas base (referencia/arranque):** se puede prototipar sobre Bullet/PhysX, pero el objetivo es motor propio en L2 para controlar fractura y acoplamiento músculo-hueso.
- **Scripting/experimentos:** Python vía pybind11 (configurar escenarios, correr validaciones, tunear parámetros sin recompilar).
- **Rendering:** Vulkan (o bgfx para arrancar rápido). Debug-draw propio desde el día 1.
- **Build:** CMake + vcpkg.
- **Tests:** GoogleTest (C++) + pytest (Python) para validación numérica.
- **Datos anatómicos:** modelos OpenSim (huesos, rutas musculares, brazos de momento), Visible Human Project / BodyParts3D para geometría. Constantes (densidad ósea, PCSA muscular, volúmenes de cámara cardíaca) en Parameter DB versionada.

## 8. Validación — el tip #1

Cada sistema debe poder **probarse contra la realidad**:

- Músculo Hill → curvas fuerza-longitud y fuerza-velocidad publicadas.
- Marcha → comparar cinemática (ángulos articulares) y GRF vs datos de laboratorio de marcha (p. ej. datasets de gait).
- Corazón → curva presión-volumen (bucle PV), gasto cardíaco fisiológico.
- Respiración → volúmenes pulmonares, curva de disociación de O2.
- Nervio → forma y velocidad del potencial de acción.

El proyecto incluye un **arnés de validación** (L9) que corre estos benchmarks y falla el build si un sistema se desvía de rangos fisiológicos. Sin esto, no sabemos si "funciona".

## 9. Estructura de carpetas

```
soma/
├── docs/
├── data/                  # Parameter DB, geometrías anatómicas, datasets de validación
├── engine/
│   ├── l0_foundation/
│   ├── l1_core/
│   ├── l2_physics/
│   ├── l3_anatomy/
│   ├── l4_physiology/
│   ├── l5_nervous/
│   ├── l6_sensory/
│   ├── l7_brain/
│   ├── l8_render/
│   └── l9_tools/
├── bindings/              # pybind11
├── scenarios/             # escenarios en Python (mundo, gravedad, objetos)
├── tests/                 # unit + validación fisiológica
└── CMakeLists.txt
```

Detalle módulo a módulo en [MODULES.md](MODULES.md). Orden de construcción en [ROADMAP.md](ROADMAP.md).
