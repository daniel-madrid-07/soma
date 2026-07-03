// SOMA — L6 — propiocepción articular.
//
// Mide el ángulo real de una articulación y su velocidad a partir del estado
// mecánico del hueso (como hacen los receptores articulares y los husos). Es
// información SENSORIAL: el controlador motor solo conocerá la postura por esta
// vía, nunca leyendo directamente variables de física ajenas.
#pragma once

#include "physics/constraints/joint_limit.hpp"  // signed_angle
#include "physics/rigid/body.hpp"

namespace soma::sensory {

using math::Real;
using math::Vec3;

struct JointAngleSense {
    Vec3 axis{0, 1, 0};       // eje de la articulación (mundo)
    Vec3 ref{1, 0, 0};        // dirección de ángulo cero (mundo)
    Vec3 local_dir{1, 0, 0};  // dirección solidaria al hueso distal

    // Ángulo articular (rad).
    Real angle(const physics::RigidBody& distal) const {
        return physics::signed_angle(axis, ref, distal.orient.rotate(local_dir));
    }
    // Velocidad angular sobre el eje (rad/s).
    Real rate(const physics::RigidBody& distal) const {
        return math::dot(distal.omega, axis);
    }
};

}  // namespace soma::sensory
