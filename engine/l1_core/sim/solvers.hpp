// SOMA — L1 — integradores ODE.
// Un solo lugar para la integración numérica; ningún sistema la reimplementa.
// - Euler semi-implícito (simpléctico): estable y barato para cuerpos rígidos.
// - RK4: cuando importa la precisión (dinámica muscular, química, nervio).
#pragma once

#include "core/math/scalar.hpp"
#include "core/math/vec.hpp"

namespace soma::solve {

using soma::math::Real;
using soma::math::Vec3;

// Euler semi-implícito para una partícula. accel = f(pos, vel).
// Actualiza velocidad con la aceleración y LUEGO posición con la nueva velocidad.
template <class AccelFn>
void semi_implicit_euler(Vec3& pos, Vec3& vel, AccelFn accel, Real dt) {
    Vec3 a = accel(pos, vel);
    vel += a * dt;
    pos += vel * dt;
}

// RK4 genérico sobre un espacio de estados con operator+ y operator*(Real).
// f(t, y) -> dy/dt. Sirve para Vec3, double, o estados compuestos.
template <class State, class Deriv>
State rk4(const State& y, Real t, Real dt, Deriv f) {
    State k1 = f(t, y);
    State k2 = f(t + dt * 0.5, y + k1 * (dt * 0.5));
    State k3 = f(t + dt * 0.5, y + k2 * (dt * 0.5));
    State k4 = f(t + dt, y + k3 * dt);
    return y + (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (dt / 6.0);
}

// Euler explícito (para química lenta donde la estabilidad no es crítica).
template <class State, class Deriv>
State euler(const State& y, Real t, Real dt, Deriv f) {
    return y + f(t, y) * dt;
}

}  // namespace soma::solve
