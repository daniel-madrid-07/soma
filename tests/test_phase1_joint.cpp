// SOMA — Test FASE 1 (slice 2): articulación esférica.
//
// Un "hueso" (barra) cuelga de un anclaje fijo por una articulación de rótula y
// oscila bajo gravedad. Verifica que la articulación MANTIENE unidos los huesos
// (el punto de anclaje no se separa) y que el movimiento es físico y acotado
// (no gana energía). Es el ladrillo del esqueleto articulado (ragdoll).
#include "physics/constraints/ball_joint.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"

#include <cassert>
#include <cmath>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using physics::BallJoint;

constexpr Vec3 kGravity{0, 0, -9.80665};
const Vec3 kAnchor{0, 0, 5};

static Real dist(Vec3 a, Vec3 b) { return math::length(a - b); }

int main() {
    // Anclaje fijo al mundo (cuerpo estático) en (0,0,5).
    RigidBody anchor = RigidBody::make_static(kAnchor);

    // Barra de 1 m a lo largo de X, masa 1 kg. CM en (0.5,0,5) para que su
    // extremo local (-0.5,0,0) coincida con el anclaje. Arranca horizontal.
    RigidBody bar = RigidBody::make_box(1.0, Vec3{0.5, 0.05, 0.05}, Vec3{0.5, 0, 5});

    BallJoint joint;
    joint.local_a = Vec3{-0.5, 0, 0};  // extremo de la barra
    joint.local_b = Vec3{0, 0, 0};     // en el anclaje

    Real dt = 1.0 / 1000.0;
    Real max_anchor_error = 0.0;
    Real max_reach = 0.0;

    for (int step = 0; step < 6000; ++step) {   // 6 s
        physics::apply_gravity(bar, kGravity);
        bar.integrate_velocity(dt);
        for (int it = 0; it < 10; ++it)          // impulsos secuenciales
            physics::solve_ball(bar, anchor, joint, dt);
        bar.integrate_position(dt);

        // El extremo anclado de la barra debe seguir pegado al anclaje.
        Vec3 world_anchor = bar.pos + bar.orient.rotate(joint.local_a);
        max_anchor_error = std::max(max_anchor_error, dist(world_anchor, kAnchor));
        // El CM nunca debe alejarse más que la longitud del brazo (~0.5 m).
        max_reach = std::max(max_reach, dist(bar.pos, kAnchor));
    }

    // La articulación mantuvo la unión: error de anclaje sub-milimétrico.
    assert(max_anchor_error < 2e-3);
    // Movimiento acotado: no explotó ni ganó energía (brazo ≈ 0.5 m).
    assert(max_reach < 0.55);
    // Cayó: partió horizontal (z=5) y ahora cuelga por debajo del anclaje.
    assert(bar.pos.z < 5.0);

    return 0;  // PASS si no saltó ningún assert
}
