// SOMA — L5 — reflejo miotático (de estiramiento).
//
// El huso muscular detecta estiramiento y velocidad de estiramiento; por un arco
// reflejo en la médula excita al PROPIO músculo. Resultado: el músculo resiste
// ser alargado → rigidez y estabilidad postural, sin intervención del cerebro.
// Es feedback local, rápido y automático.
#pragma once

#include "core/math/scalar.hpp"

namespace soma::nervous {

using math::Real;

struct StretchReflex {
    Real l_rest = 0.5;   // longitud de referencia del músculo (m)
    Real k_len = 2.5;    // ganancia por estiramiento (huso, componente estático)
    Real k_vel = 0.05;   // ganancia por velocidad de estiramiento (componente dinámico)

    // Activación refleja [0..1] a partir del estado del músculo.
    // Solo excitatoria (un músculo solo tira): si no está estirado, no hay reflejo.
    Real activation(Real length, Real velocity) const {
        Real a = k_len * (length - l_rest) + k_vel * velocity;
        if (a < 0.0) return 0.0;
        if (a > 1.0) return 1.0;
        return a;
    }
};

}  // namespace soma::nervous
