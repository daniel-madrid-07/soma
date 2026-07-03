// SOMA — L2 — gravedad. Añade peso (m·g) al acumulador de fuerza de un cuerpo.
#pragma once

#include "physics/rigid/body.hpp"

namespace soma::physics {

// g típico: {0,0,-9.80665}. Los cuerpos estáticos (inv_mass=0) se ignoran solos.
inline void apply_gravity(RigidBody& b, Vec3 g) {
    if (b.inv_mass == 0.0) return;
    b.apply_force_cm(g * b.mass);
}

}  // namespace soma::physics
