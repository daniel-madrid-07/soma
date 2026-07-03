// SOMA — L5 — osciladores de fase acoplados (coordinación inter-articular).
//
// Cada articulación tiene un oscilador de fase; el acoplamiento los bloquea a un
// DESFASE deseado respecto de la articulación líder. Así emerge la coordinación de
// una zancada: la rodilla flexiona con un desfase fijo respecto de la cadera.
// (Modelo de CPG por fases, estándar en locomoción — p. ej. Ijspeert.)
//
//   dφ_i/dt = ω + k · sin( (φ_0 + desfase_i) − φ_i )      (i > 0)
//   dφ_0/dt = ω                                            (líder)
//
// La salida de cada oscilador se reparte en flexor/extensor en antifase.
#pragma once

#include "core/math/scalar.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace soma::control {

using math::Real;

struct CoupledOscillators {
    Real omega = 2.0 * math::Pi;   // frecuencia base (rad/s) ~1 Hz
    Real k = 8.0;                  // ganancia de acoplamiento
    std::vector<Real> phase;       // fase por articulación
    std::vector<Real> offset;      // desfase deseado respecto de la líder (índice 0)

    explicit CoupledOscillators(std::size_t n = 2) : phase(n, 0.0), offset(n, 0.0) {}

    void step(Real dt) {
        std::size_t n = phase.size();
        std::vector<Real> dphi(n);
        dphi[0] = omega;   // la articulación 0 marca el paso
        for (std::size_t i = 1; i < n; ++i) {
            Real target = phase[0] + offset[i];
            dphi[i] = omega + k * std::sin(target - phase[i]);
        }
        for (std::size_t i = 0; i < n; ++i) {
            phase[i] += dphi[i] * dt;
            if (phase[i] > 2 * math::Pi) phase[i] -= 2 * math::Pi;
            else if (phase[i] < 0) phase[i] += 2 * math::Pi;
        }
    }

    Real flexor_drive(std::size_t i) const {
        Real s = std::sin(phase[i]);
        return s > 0 ? s : 0;
    }
    Real extensor_drive(std::size_t i) const {
        Real s = std::sin(phase[i]);
        return s < 0 ? -s : 0;
    }
};

// Diferencia de fase envuelta a (−π, π].
inline Real wrap_pi(Real a) {
    while (a > math::Pi) a -= 2 * math::Pi;
    while (a <= -math::Pi) a += 2 * math::Pi;
    return a;
}

}  // namespace soma::control
