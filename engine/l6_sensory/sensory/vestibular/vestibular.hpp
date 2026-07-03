// SOMA — L6 — sistema vestibular.
//
// Otolitos: detectan la dirección de "arriba" del cuerpo respecto de la gravedad
//   → inclinación (tilt) frente a la vertical.
// Canales semicirculares: detectan velocidad angular de la cabeza/torso.
// El controlador de equilibrio SOLO conocerá su postura por esta vía sensorial
// (más la propiocepción), nunca leyendo la física del mundo. Así el equilibrio es
// una consecuencia real de sentir y corregir, no un truco.
#pragma once

#include "physics/rigid/body.hpp"

#include <cmath>

namespace soma::sensory {

using math::Real;
using math::Vec3;

struct Vestibular {
    Vec3 up_local{0, 0, 1};   // eje "arriba" solidario al cuerpo

    // Inclinación con signo en el plano sagital (XZ): 0 = vertical, + = hacia +X.
    // (Los otolitos comparan 'arriba' del cuerpo con la vertical gravitatoria.)
    Real tilt(const physics::RigidBody& body) const {
        Vec3 up = body.orient.rotate(up_local);
        return std::atan2(up.x, up.z);
    }

    // Velocidad angular sobre el eje de cabeceo (+Y). (Canal semicircular.)
    Real pitch_rate(const physics::RigidBody& body) const { return body.omega.y; }
};

}  // namespace soma::sensory
