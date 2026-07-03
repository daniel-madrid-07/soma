// SOMA — L2 — articulación esférica (point-to-point) por impulsos secuenciales.
//
// Mantiene coincidentes dos puntos de anclaje (uno en cada hueso). Es la base de
// la articulación de rótula (hombro, cadera). El límite anatómico de rango se
// añade encima (ver joint_limit.hpp). Un extremo puede anclarse al mundo con un
// RigidBody estático.
//
// Método: restricción a nivel de velocidad + corrección de deriva (Baumgarte).
// Estable y barato; el LOD científico usará coordenadas reducidas (Featherstone).
#pragma once

#include "physics/rigid/body.hpp"

namespace soma::physics {

// Matriz antisimétrica: skew(w)·r = w × r.
inline Mat3 skew(Vec3 w) {
    return Mat3{{0, w.z, -w.y}, {-w.z, 0, w.x}, {w.y, -w.x, 0}};
}

struct BallJoint {
    Vec3 local_a{0, 0, 0};  // anclaje en el marco local de A
    Vec3 local_b{0, 0, 0};  // anclaje en el marco local de B
    Real beta = 0.2;        // ganancia de corrección de deriva
};

// Resuelve una iteración de la restricción entre A y B (cualquiera puede ser estático).
inline void solve_ball(RigidBody& A, RigidBody& B, const BallJoint& j, Real dt) {
    Mat3 RA = A.orient.to_mat3();
    Mat3 RB = B.orient.to_mat3();
    Vec3 rA = RA * j.local_a;               // brazo desde CM de A al anclaje (mundo)
    Vec3 rB = RB * j.local_b;
    Vec3 pA = A.pos + rA;                   // anclaje en A (mundo)
    Vec3 pB = B.pos + rB;
    Vec3 C = pB - pA;                       // error de posición (deriva)

    Mat3 invIA = A.inv_inertia_world();
    Mat3 invIB = B.inv_inertia_world();

    // Masa efectiva: K = (invMA+invMB)I − skew(rA)·invIA·skew(rA) − skew(rB)·invIB·skew(rB).
    Mat3 sA = skew(rA), sB = skew(rB);
    Mat3 K = Mat3::diagonal(A.inv_mass + B.inv_mass,
                            A.inv_mass + B.inv_mass,
                            A.inv_mass + B.inv_mass)
             + (sA * invIA * sA) * (-1.0)
             + (sB * invIB * sB) * (-1.0);

    // Velocidad relativa del punto de anclaje.
    Vec3 vA = A.vel + math::cross(A.omega, rA);
    Vec3 vB = B.vel + math::cross(B.omega, rB);
    Vec3 Cdot = vB - vA;

    // Impulso: P = −K⁻¹ (Cdot + β/dt · C).
    Vec3 bias = C * (j.beta / dt);
    Vec3 P = inverse(K) * (-(Cdot + bias));

    // Aplica ±P (tercera ley de Newton).
    A.vel -= P * A.inv_mass;
    A.omega -= invIA * math::cross(rA, P);
    B.vel += P * B.inv_mass;
    B.omega += invIB * math::cross(rB, P);
}

}  // namespace soma::physics
