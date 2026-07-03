// SOMA — L2 — límite angular anatómico (rango de movimiento).
//
// Un codo no hiperextiende; una rodilla no se dobla hacia adelante. Este límite
// restringe el ángulo de giro de un hueso, alrededor de un eje, a [lo, hi] rad.
// Se resuelve como restricción de desigualdad a nivel de velocidad + Baumgarte.
// Se aplica ENCIMA de la articulación (ball_joint fija el punto; esto limita el rango).
//
// Aquí se limita un cuerpo respecto a una referencia del MUNDO (el hueso proximal
// fijo). La versión hueso-hueso generaliza el mismo cálculo al marco del proximal.
#pragma once

#include "physics/rigid/body.hpp"

namespace soma::physics {

struct AngleLimit1D {
    Vec3 axis{0, 1, 0};    // eje de giro (mundo, unitario)
    Vec3 ref{1, 0, 0};     // dirección de referencia = ángulo 0 (mundo, unitario)
    Vec3 local_dir{1, 0, 0};  // dirección solidaria al hueso cuyo ángulo se mide
    Real lo = -3.15;       // límite inferior (rad)
    Real hi = 3.15;        // límite superior (rad)
    Real beta = 0.2;       // corrección de deriva
};

// Ángulo con signo de 'r' respecto de 'ref' alrededor de 'axis'.
inline Real signed_angle(Vec3 axis, Vec3 ref, Vec3 r) {
    Real s = math::dot(axis, math::cross(ref, r));
    Real c = math::dot(ref, r);
    return std::atan2(s, c);
}

// Resuelve una iteración del límite sobre el cuerpo b. No hace nada si está dentro.
inline void solve_angle_limit(RigidBody& b, const AngleLimit1D& L, Real dt) {
    if (b.inv_mass == 0.0) return;
    Vec3 r = b.orient.rotate(L.local_dir);
    Real theta = signed_angle(L.axis, L.ref, r);

    Real violation;   // >0 fuera por arriba, <0 fuera por abajo
    int dir;
    if (theta > L.hi)      { violation = theta - L.hi; dir = +1; }
    else if (theta < L.lo) { violation = theta - L.lo; dir = -1; }
    else return;  // dentro del rango: articulación libre

    Mat3 invI = b.inv_inertia_world();
    Real eff = math::dot(L.axis, invI * L.axis);   // masa efectiva angular sobre el eje
    if (eff < math::Eps) return;

    Real wa = math::dot(b.omega, L.axis);           // velocidad angular sobre el eje
    Real desired = -(L.beta / dt) * violation;      // objetivo: empujar de vuelta al rango
    Real lambda = (desired - wa) / eff;

    // Un tope solo empuja hacia dentro; nunca acelera hacia fuera.
    if (dir > 0 && lambda > 0) lambda = 0.0;
    if (dir < 0 && lambda < 0) lambda = 0.0;

    b.omega += invI * (L.axis * lambda);
}

}  // namespace soma::physics
