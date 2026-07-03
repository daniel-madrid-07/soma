// SOMA — L2 — contacto con el suelo (plano infinito, normal +z).
//
// Método de penalización (muelle-amortiguador): al penetrar, el suelo empuja con
// fuerza proporcional a la penetración y disipa con amortiguación. Es la reacción
// del suelo (GRF) que sentirá el pie al caminar. Estable a 1 kHz.
//
// NOTA: es el modelo de contacto de arranque (LOD tiempo real). El solver de
// contactos por impulsos/restricciones (LOD completo) llega en physics/constraints.
#pragma once

#include "physics/rigid/body.hpp"

namespace soma::physics {

struct GroundPlane {
    Real height = 0.0;      // z del suelo
    Real stiffness = 5.0e4; // N/m
    Real damping = 3.0e2;   // N·s/m (normal)
    Real friction = 2.0e2;  // amortiguación tangencial (N·s/m)
    Real mu = 0.8;          // coef. de fricción (limita la fuerza tangencial)
};

// Aplica la reacción del suelo a un cuerpo. Devuelve la magnitud de la fuerza
// normal (GRF) — útil para sensores plantares más adelante.
inline Real resolve_ground(RigidBody& b, const GroundPlane& g) {
    if (b.inv_mass == 0.0) return 0.0;
    Vec3 n{0, 0, 1};
    Real contact_z = g.height + b.radius;        // el borde inferior toca el suelo aquí
    Real penetration = contact_z - b.pos.z;
    if (penetration <= 0.0) return 0.0;          // sin contacto

    Vec3 cp = b.pos - n * b.radius;              // punto de contacto
    Vec3 v = b.point_velocity(cp);
    Real vn = math::dot(v, n);

    // Normal: muelle + amortiguación, solo empuja (>= 0).
    Real fn = g.stiffness * penetration - g.damping * vn;
    if (fn < 0.0) fn = 0.0;

    // Tangencial: amortiguación de deslizamiento, acotada por Coulomb (mu·fn).
    Vec3 vt = v - n * vn;
    Vec3 ft = vt * (-g.friction);
    Real ft_mag = math::length(ft);
    Real ft_max = g.mu * fn;
    if (ft_mag > ft_max && ft_mag > math::Eps) ft = ft * (ft_max / ft_mag);

    b.apply_force_at(n * fn + ft, cp);
    return fn;
}

// Contacto de un PUNTO concreto del cuerpo (p. ej. la planta del pie en el extremo
// distal de la tibia), no del centro de masa. Igual modelo de penalización + fricción.
// Devuelve la fuerza normal (GRF) — señal para los sensores plantares.
inline Real resolve_ground_point(RigidBody& b, Vec3 world_point, Real foot_radius,
                                 const GroundPlane& g) {
    if (b.inv_mass == 0.0) return 0.0;
    Vec3 n{0, 0, 1};
    Real contact_z = g.height + foot_radius;
    Real penetration = contact_z - world_point.z;
    if (penetration <= 0.0) return 0.0;

    Vec3 v = b.point_velocity(world_point);
    Real vn = math::dot(v, n);
    Real fn = g.stiffness * penetration - g.damping * vn;
    if (fn < 0.0) fn = 0.0;

    Vec3 vt = v - n * vn;
    Vec3 ft = vt * (-g.friction);
    Real ft_mag = math::length(ft);
    Real ft_max = g.mu * fn;
    if (ft_mag > ft_max && ft_mag > math::Eps) ft = ft * (ft_max / ft_mag);

    b.apply_force_at(n * fn + ft, world_point);
    return fn;
}

}  // namespace soma::physics
