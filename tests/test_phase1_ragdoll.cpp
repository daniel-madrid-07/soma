// SOMA — Test FASE 1 (capstone): esqueleto articulado (ragdoll).
//
// Cadena de 3 "huesos" unidos por articulaciones de rótula, colgando de un
// anclaje fijo y oscilando bajo gravedad. Demuestra articulación multi-cuerpo:
// todas las articulaciones mantienen la unión y el movimiento es físico y acotado.
// Junto con test_phase1_rigid (reposo sobre el suelo) cubre el hito de Fase 1.
#include "physics/constraints/ball_joint.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"

#include <cassert>
#include <cmath>
#include <vector>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using physics::BallJoint;

constexpr Vec3 kGravity{0, 0, -9.80665};
const Vec3 kAnchor{0, 0, 5};

// Una articulación entre el cuerpo 'a' y el cuerpo 'b' (b < 0 => el anclaje fijo).
struct Link { int a; int b; BallJoint joint; };

static Real dist(Vec3 p, Vec3 q) { return math::length(p - q); }

int main() {
    RigidBody anchor = RigidBody::make_static(kAnchor);

    // 3 huesos de 0.6 m a lo largo de X, encadenados desde el anclaje.
    std::vector<RigidBody> bones;
    for (int i = 0; i < 3; ++i) {
        Real cx = 0.3 + 0.6 * i;             // CM de cada hueso
        bones.push_back(RigidBody::make_box(1.0, Vec3{0.3, 0.05, 0.05}, Vec3{cx, 0, 5}));
    }

    std::vector<Link> links;
    // Hueso 0 <-> anclaje.
    links.push_back({0, -1, BallJoint{Vec3{-0.3, 0, 0}, Vec3{0, 0, 0}, 0.2}});
    // Hueso i <-> hueso i-1.
    links.push_back({1, 0, BallJoint{Vec3{-0.3, 0, 0}, Vec3{0.3, 0, 0}, 0.2}});
    links.push_back({2, 1, BallJoint{Vec3{-0.3, 0, 0}, Vec3{0.3, 0, 0}, 0.2}});

    Real dt = 1.0 / 1000.0;
    Real max_link_error = 0.0;
    Real max_reach = 0.0;

    for (int step = 0; step < 8000; ++step) {           // 8 s
        for (auto& b : bones) physics::apply_gravity(b, kGravity);
        for (auto& b : bones) b.integrate_velocity(dt);

        // Gauss-Seidel sobre todas las articulaciones.
        for (int it = 0; it < 20; ++it) {
            for (auto& L : links) {
                RigidBody& A = bones[L.a];
                RigidBody& B = (L.b < 0) ? anchor : bones[L.b];
                physics::solve_ball(A, B, L.joint, dt);
            }
        }

        for (auto& b : bones) b.integrate_position(dt);

        // Error de cada articulación (los dos anclajes deben coincidir).
        for (auto& L : links) {
            RigidBody& A = bones[L.a];
            RigidBody& B = (L.b < 0) ? anchor : bones[L.b];
            Vec3 pa = A.pos + A.orient.rotate(L.joint.local_a);
            Vec3 pb = B.pos + B.orient.rotate(L.joint.local_b);
            max_link_error = std::max(max_link_error, dist(pa, pb));
        }
        for (auto& b : bones)
            max_reach = std::max(max_reach, dist(b.pos, kAnchor));
    }

    // Todas las articulaciones mantuvieron la unión (error milimétrico).
    assert(max_link_error < 5e-3);
    // La cadena nunca se estiró más allá de su longitud (~1.8 m + medio hueso).
    assert(max_reach < 2.0);
    // La cadena cayó: al menos el hueso final quedó por debajo del anclaje.
    assert(bones[2].pos.z < 5.0);

    return 0;
}
