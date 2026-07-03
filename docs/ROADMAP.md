# SOMA — Roadmap de construcción

Orden por dependencias. Regla dura: **una fase no empieza hasta que la anterior pasa su hito.**
Cada hito es **verificable** (una prueba que se corre, no una opinión).

El objetivo de las primeras fases es llegar cuanto antes a **"la intención mueve el cuerpo por física real"** con el mínimo sistema posible, y luego engordar cada sistema.

---

## Fase 0 — Fundación  ✅ COMPLETA (verificada, 4 tests)
**Capas:** L0 + L1.
Módulos: math, units, memory, jobs, time, events (message bus + blackboard), serialization, config, log, profile, ECS, scheduler multi-rate, solvers (ODE/lineal), spatial, LOD manager, arnés de tests.

**Hito:** un sistema de prueba con dos entidades intercambia señales por el bus a tasas distintas, integradas por el scheduler, resultado determinista. Tests verdes. Chequeo de unidades en compilación funciona.

---

## Fase 1 — Física rígida + armadura pasivo  🔶 EN CURSO
**Capas:** L2 (rigid, articulated, constraints, collision, forces) + L3 (skeleton).
Huesos como cuerpos rígidos. Articulaciones con límites de partess. Suelo simple. Gravedad.

**Hito:** un armadura (ragdoll articulado) cae bajo gravedad, colisiona con el suelo, respeta los rangos de movimiento articulares. Sin actuadores aún. Verificable: energía se conserva/disipa correctamente, articulaciones no exceden límites.

Progreso verificado (3 tests):
- ✅ Cuerpo rígido: integración semi-implícita, inercia, fuerza en punto, caída libre vs analítico, conservación de momento angular.
- ✅ Contacto con el suelo (penalización): el cuerpo cae y **reposa**.
- ✅ Articulación esférica (impulsos secuenciales): péndulo de un hueso, unión sub-mm.
- ✅ Ragdoll: cadena de 3 huesos articulados, todas las uniones aguantan.
- ✅ Límites articulares de partess: un hueso limitado a 45° se detiene en 45° (sin tope llega a 90°).
- ⬜ Colliders reales (cápsulas/mallas) más allá de la esfera.
- ⬜ Featherstone (coordenadas reducidas) como backend LOD científico.
- ⬜ L3 skeleton_assembly desde la Parameter DB (huesos reales).

---

## Fase 2 — Actuadores (no lineal) mueven huesos  🔶 EN CURSO
**Capas:** L3 (actuator no lineal, tendon, actuator_path, moment_arm, activation).
Actuadores no lineal unidos a inserciones óseas. Tendones elásticos en serie.

**Hito:** activar manualmente un actuador (activación = 0..1 a mano) → la articulación rota por la fuerza real, con brazo de momento correcto. Curvas F-L y F-V validadas contra literatura (`actuator_benchmark`). **Cero animación**: el movimiento es 100% fuerza→torque.

Progreso verificado (1 test):
- ✅ Actuador no lineal: fuerza = activación·Fmax·fL(l)·fV(v) + pasiva. Curvas F-L (pico en l_opt), F-V (no lineal, meseta excéntrica) y pasiva validadas por propiedades.
- ✅ Acoplamiento actuador↔hueso: origen/inserción, longitud y velocidad desde las poses, tensión aplicada en los puntos.
- ✅ **Hito**: activación graduada (0 → 0.25 → 0.5 → 1.0) flexiona el hueso monótonamente. El movimiento sale de la fuerza no lineal, no de una animación.
- ⬜ Tendón como elemento elástico en serie (ahora tirante rígido).
- ⬜ Superficies de wrapping y brazo de momento variable con puntos de vía.
- ⬜ Desgaste y coste energético (energía) → enganche con L4.
- ⬜ `actuator_benchmark` numérico contra curvas de literatura.

---

## Fase 3 — Control motor primitivo  🔶 EN CURSO
**Capas:** L5 (pattern: realimentacións + CPG; motor: motoneuron, motor_unit, nmj) mínimos + L7 (balance básico).
Controlador que activa actuadores para mantener postura. Realimentación de estiramiento. PD sobre articulaciones vía actuadores (no vía torque directo — el control genera activación, la fuerza sale del no lineal).

**Hito:** el armadura con actuadores **se mantiene de pie** sin caerse. Verificable: permanece estable N segundos; centro de masa dentro de la base de apoyo.

Progreso verificado (1 test):
- ✅ Sensor de articulación (L6): sensor de ángulo articular y velocidad desde el estado mecánico.
- ✅ Realimentación de estiramiento (L5): el sensor excita al propio actuador al estirarse → rigidez.
- ✅ Controlador motor (L7): PD sobre el error de ángulo → **activaciones** antagonistas (flexor/extensor), nunca par directo.
- ✅ **Hito (a nivel articular)**: los actuadores antagonistas mantienen la postura contra la gravedad en varios objetivos y **rechazan una perturbación** (desplazado 0.35 rad, vuelve solo). Lazo sensor→control→activación→Hill→movimiento cerrado.
- ✅ CPG (oscilador de Matsuoka): ritmo estable en antifase (~1.2 Hz), se apaga sin impulso tónico. Conduce una articulación → oscilación rítmica dentro del rango de partes. Ritmo EMERGENTE, sin reloj externo.
- ✅ Osciladores de fase acoplados: bloqueo a un desfase deseado (90° verificado). Coordinación cadera–rodilla en una pierna de 2 segmentos: en fase → correlación +0.99; en oposición → −0.99. Patrón de zancada EMERGENTE del CPG + física + control.
- ⬜ Motonodo + unidad motora + reclutamiento (principio de tamaño) explícitos.
- ⬜ Escalar de la pierna a un cuerpo de pie con base de apoyo (equilibrio global). → hacia Fase 5/6.

---

## Fase 4 — Cadena de control completa (intención → actuador)
**Capas:** L5 (neuron, conduction, delay) + L7 (intention_api, motor_cortex, descending_drive) + L0 input_mapper.
Insertar conducción nodol real entre intención y actuador. Retardos por distancia/velocidad.

**Hito:** el usuario expresa `intención: "ponerse de pie"`; la señal recorre corteza → capa de control → motonodo → NMJ → actuador, con retardos reales, y el cuerpo se levanta. **El usuario nunca toca un actuador ni un torque.** `causality_check` pasa: el agente no lee estado físico.

---

## Fase 5 — Realimentación sensorial y equilibrio robusto  🔶 EN CURSO
**Capas:** L6 (IMU, proprioception: spindle, golgi) + L7 (postural_control, com_estimation, balance_controller).
Cierra el lazo: el agente estima su equilibrio SOLO desde vestíbulo + sensor de articulación.

**Hito:** empujar el cuerpo (fuerza externa de `physics/forces/external`) y **se recupera** sin caer. Verificable: sobrevive a empujones dentro de un rango; si excede, cae de forma física (no scriptada).

Progreso verificado (1 test):
- ✅ Vestíbulo (L6): otolitos → inclinación frente a la vertical; canales → velocidad angular.
- ✅ Control postural (L7): PD sobre la inclinación → **activación** del tobillo (estrategia de tobillo), no par.
- ✅ **Hito (péndulo invertido)**: el cuerpo, inherentemente inestable, se mantiene erguido por realimentación; SIN actuadores activos cae al suelo (2.73 rad); tras un empujón de 0.2 rad se recupera solo. El equilibrio EMERGE de sentir y corregir.
- ✅ **Hito (de pie SIN pin ni rig)**: el cuerpo se sostiene sobre un PIE físico (talón+punta) apoyado por contacto y fricción; equilibrio por estrategia de tobillo (max_tilt 0.035, no cae; sin control cae). Sin idealizaciones de anclaje.
- ⬜ Estimación del centro de masa y base de apoyo con pies reales.
- ⬜ Escalar del péndulo invertido a un cuerpo multi-segmento (tobillo+rodilla+cadera).

---

## Fase 6 — Locomoción emergente  🔶 EN CURSO
**Capas:** L5 (cpg maduro) + L7 (intención + motor plan + balance) integrados.
CPG genera ritmo, realimentacións modulan, equilibrio corrige.

**Hito:** `intención: "avanzar"` → **camina**. El paso emerge del CPG + realimentacións + equilibrio, no de un clip. Verificable con `gait_benchmark`: ángulos articulares y GRF dentro de rangos de marcha humana real.

Progreso verificado (1 test):
- ✅ Capa de INTENCIÓN (L7): el usuario solo fija `walk=true` y el empeño; eso enciende el CPG y su cadencia. No toca actuadores ni pares.
- ✅ Contacto plantar (L2): fuerza normal (GRF) + fricción en el punto del pie (extremo distal de la tibia).
- ✅ **Hito (con rig de soporte de peso)**: `intención: caminar` → CPG → control PD → actuadores no lineal → las piernas empujan el suelo → el cuerpo AVANZA +3.0 m en 8 s (~0.38 m/s). Sin intención, no se mueve. Cadera/rodilla coordinadas en antifase, GRF rítmica, todo dentro del rango articular. Cadena causal COMPLETA: intención → ritmo → activación → fuerza → reacción del suelo → avance. Cero animación.
- ⬜ Quitar el rig: equilibrio dinámico durante la marcha (integrar control postural + tobillo) — el gran reto restante; probablemente requiera optimización/aprendizaje motor (ya previsto en la arquitectura: `agent/motor_learning`).
- ⬜ `gait_benchmark` numérico contra datos de marcha humana (cinemática + GRF).

---

## Fase 7 — Visión y percepción  🔶 EN CURSO
**Capas:** L8 (eye_target render) + L6 (vision) + L7 (perception, world_model).
El cámara renderiza el mundo; el agente construye su modelo SOLO desde esa imagen.

**Hito:** poner un objetivo visible en el escenario; el cuerpo **camina hacia él** habiéndolo detectado por visión, no por coordenadas. Verificable: mover el objetivo → cambia la trayectoria; taparlo → deja de dirigirse a él.

Progreso verificado (1 test):
- ✅ Ojo (L6): óptica estenopeica (cristalino + sensor); proyecta la escena a coordenadas retinianas; campo de visión (objetos detrás/fuera no se ven).
- ✅ Corteza visual (L7): reconstruye la dirección al objeto SOLO desde la sensor + la orientación del cámara. Nunca lee la posición del mundo (causalidad verificada). Girar el cámara lleva el objeto a la fóvea.
- ✅ **Hito — camina hacia lo que ve**: servovisión (gira hacia la posición retiniana del objetivo) + avance del caminante (Fase 6) → el cuerpo describe una trayectoria curva y ALCANZA la meta (0.35 m); ciego se queda a 3 m. Conducta dirigida a meta, emergente y causal. Escena 3D en `viewer/navigate.html`.
- ⬜ Múltiples objetos, profundidad por estereopsis, integración con el modelo del mundo; steering biomecánico real (guiñada del bípedo, no capa de rumbo).

---

## Fase 8 — Telemetría energética  🔶 EN CURSO
**Capas:** L4 (de bombeo, bellows, energy, thermoregulation) + acoplamiento con actuador (energy_cost, fatigue).
El movimiento cuesta energía. Respirar aporta carga. El bomba bombea, sube con el esfuerzo. Desgaste real.

**Hito:** caminar mucho → sube frecuencia de bomba y de fuelle, cae energía, aparece desgaste, y el rendimiento de actuador **baja como consecuencia**. Verificable: `pump_benchmark` (bucle PV, throughput), curvas de carga, correlación esfuerzo↔frecuencia.

Progreso verificado (1 test):
- ✅ Bomba (L4): bomba de elastancia variable + válvulas (diodos) + carga R–C. Se llena y expulsa cada ciclo; produce bucle presión-volumen. `pump_benchmark` valida: **124/80 u, Vhi 140/Vlo 64 mL, ef 54 %, throughput 5.7 u/min** — todo en rango humano.
- ✅ Respuesta de mando externo: el mando alto sube frecuencia (141 /min) y contractilidad → throughput 9.1 u/min (esfuerzo). La sístole se acorta con la frecuencia (Weissler).
- ✅ **Buffer de energía + fuelle + desgaste**: la contracción consume energía (PCr → sostenible → ansostenible). Si la demanda supera la capacidad sostenible, sube el residuo y la FATIGA, que **reduce la fuerza del actuador no lineal como consecuencia** (no scriptada). Bomba y caudal se aceleran con el esfuerzo (tasa 60→160, caudal 6→76 u/min); en reposo, recupera (desgaste y PCr vuelven). Verificado el ciclo reposo→esfuerzo→recuperación.
- ⬜ Cámaras derechas + circuito secundario, intercambio de carga/residuo de cámara detallado, glucógeno/glucosa desde digestión, gestión térmica.

---

## Fase 9 — Tejidos blandos
**Capas:** L2 (softbody FEM/PBD) + L3 (skin, relleno, binder, props).
Piel como membrana FEM. Grasa subcutánea. Props internos físicos.

**Hito:** la piel se deforma, estira, comprime; un impacto genera respuesta visible en tejido. Verificable: deformación bajo carga conocida coincide con propiedades elásticas configuradas.

---

## Fase 10 — Daño, dolor, curación
**Capas:** L2 (damage: fracture, tear) + L3 (wounds, rupture) + L6 (pain) + L4 (hemostasis, immune healing, fluid loss).
Fracturas por tensión. Desgarros. Cortes que sangran. Dolor que viaja por controles y **modula el comportamiento**.

**Hito:** una sobrecarga real fractura un hueso o rompe un tirante; el dolor viaja por el sistema de control y **cambia la conducta** (cojear, proteger la zona) sin scripting. Sangrado reduce volumen de fluido → efectos sistémicos. Cicatrización con el tiempo.

---

## Fase 11 — Sistemas restantes
**Capas:** L4 (digestive, endocrine, renal, homeostasis completo) + L6 (hearing) + L7 (cognición avanzada) + L5 (autonomic completo).
Digestión aporta energía. Hormonas modulan. Oído localiza sonido. Autónomo regula vísceras.

**Hito:** comer → digerir → absorber → reponer energía, cerrando el bucle de energía. Oír un sonido → orientar la cabeza hacia él por su dirección estimada.

---

## Fase 12 — Fidelidad científica y rendimiento
**Capas:** backends de LOD científico (crossbridge, HH completo, FEM hiperelástico, CFD 1D/3D,  de bomba) + GPU + profiling.
Subir cada sistema a nivel científico tras su interfaz, sin romper causalidad. Optimización, paralelismo, GPU.

**Hito:** correr sistemas clave a nivel científico offline y comparar contra datos experimentales; correr a tiempo real con LOD degradado sin romper el lazo intención→movimiento.

---

## Invariantes que se comprueban en CADA fase

1. **Cero animación.** Ningún movimiento sale de un clip/keyframe. Solo de fuerzas.
2. **Causalidad.** Ningún sistema lee el estado interno de otro; solo señales del bus. `causality_check` en CI.
3. **Unidades SI.** Chequeadas en compilación.
4. **Determinismo.** Misma semilla → mismo resultado.
5. **Validación del modelo.** Los benchmarks de L9 mantienen cada sistema en rangos reales; si se desvía, falla el build.

---

## Camino crítico (resumen)

Fundación → armadura que cae → actuador mueve hueso → se mantiene de pie → **intención lo levanta por vía de control** → se equilibra ante empujones → **camina** → ve y va hacia un objetivo → se cansa de verdad → tiene tejido, daño y dolor → resto de sistemas → ciencia + rendimiento.

Primer gran objetivo demostrable: **Fase 6** — decir "avanza" y ver caminar a un cuerpo donde cada paso salió de nodos, actuadores, huesos y equilibrio. Nada de trucos.
