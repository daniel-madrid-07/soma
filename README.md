<div align="center">

# SOMA

**A complete human being simulated from scratch — zero animations, zero keyframes, zero canned scripts. Everything emerges from physics and mechanics.**

![preview](viewer/preview.png)

</div>

---

## What it does

SOMA (Simulated Object Motion Architecture) simulates a human body built up layer by layer from
first principles: rigid-body physics, non-linear muscle actuators, a central pattern generator for
rhythm, a PD motor controller, and a vestibular/visual feedback loop. The user controls
**intention** ("walk"), not actuators or keyframes — the body does the rest.

> Nothing happens because an animation says so.
> Everything happens because a physical or mechanical system causes it.

## Key milestones

- **Walks from intention alone** — `intention: walk` → CPG → motor control → actuators → ground reaction → the body **advances at ~0.38 m/s**, validated by an automated gait benchmark (cadence, peak ground-reaction force, duty cycle). No intention, no movement.
- **Stands with no rig, no pins** — a physical foot (heel + toe, contact and friction) balances upright through vestibular feedback alone, and recovers from pushes.
- **Sees and walks toward what it sees** — reconstructs the direction to an object from a simulated camera alone (never reads its own position), then walks an emergent curved path to reach it. Blind, it doesn't.
- **Simulates its own energy cost** — a variable-elastance cardiac-pump model drives a fatigue system: sustained effort measurably drops actuator force, exactly like a real muscle running low on energy.

**22 automated tests pass** (`scripts/build_all.sh`). No animation clips exist anywhere in the project — all motion is computed from forces.

### The causal chain (the project's goal)

```
INTENTION (user: "walk")
  → CPG generates rhythm (coupled oscillators, antiphase)
  → motor control translates to ACTIVATION [0..1]  (never torque)
  → non-linear actuators generate FORCE
  → tendons/insertions apply torque to BONES
  → legs push against the GROUND
  → ground reaction (GRF + friction)
  → the body ADVANCES
  → joint sensor + vestibular system close the loop
```
Verified end to end in [tests/test_phase6_locomotion.cpp](tests/test_phase6_locomotion.cpp).

## Features

- SI units with compile-time dimensional checking, custom math library (vec/mat/quat/transform), deterministic RNG, multi-rate scheduler, ECS, ODE solvers
- Rigid-body physics: semi-implicit integration, inertia, ground contact, articulated ragdoll with joint limits
- Non-linear actuators (force-length, force-velocity, passive) coupled directly to bones
- Central pattern generator (Matsuoka oscillators) + PD motor control + vestibular/postural feedback
- Pinhole-camera vision and visuomotor navigation
- Cardiac-pump energy model with fatigue

Full build status by phase, with every verified milestone: **[docs/ROADMAP.md](docs/ROADMAP.md)**.

## Installation / Usage

### Build and verify
```bash
scripts/build_all.sh        # builds and runs all tests with g++ (C++20)
```
Requires g++ (msys2 ucrt64). With CMake installed: `cmake -B build && cmake --build build && ctest --test-dir build`.

### View the body in 3D
```bash
scripts/view.sh             # records the simulation and serves the viewers
```
Then open `http://127.0.0.1:8971/viewer/home.html` — it links all 5 viewers. The 3D body is
**driven by physics**: every pose comes from the simulation, the viewer only draws it (Three.js).

| Viewer | Shows |
|---|---|
| `realistic_nav.html` | realistic human walking through the world toward what it sees |
| `realistic.html` | rigged, photorealistic human model (Mixamo, via Three.js) — skin is real, motion is physical |
| `index.html` | gait, segment-level view |
| `navigate.html` | visuomotor navigation toward a target |
| `telemetry.html` | pump pressure-volume loop and fatigue energetics |

![real body](viewer/preview-real.png)
![vitals](viewer/preview-telemetry.png)
![navigation](viewer/preview-nav.png)

For a custom realistic model + a rendered video (Blender/MakeHuman, headless): see
[docs/BLENDER.md](docs/BLENDER.md).

## Documents

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — layers, data flow, multi-rate scheduling, LOD, technical stack
- [docs/MODULES.md](docs/MODULES.md) — complete module tree, ordered by dependency
- [docs/ROADMAP.md](docs/ROADMAP.md) — build phases with verifiable milestones

## Tech stack

C++20, CMake / g++ (msys2 ucrt64), Eigen3, EnTT, Google Test, TBB (via vcpkg), Three.js, Blender / MakeHuman (optional render pipeline)

## License

See [LICENSE](LICENSE).
