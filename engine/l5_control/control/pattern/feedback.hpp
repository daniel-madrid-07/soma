// SOMA — L5 — realimentación miotático (de estiramiento).
//
// El huso de actuador detecta estiramiento y velocidad de estiramiento; por un arco
// realimentación en la capa de control excita al PROPIO actuador. Resultado: el actuador resiste
// ser alargado → rigidez y estabilidad postural, sin intervención del agente.
// Es feedback local, rápido y automático.
#pragma once

#include "core/math/scalar.hpp"

namespace soma::control {

using math::Real;

struct FeedbackLoop {
    Real l_rest = 0.5;   // longitud de referencia del actuador (m)
    Real k_len = 2.5;    // ganancia por estiramiento (huso, componente estático)
    Real k_vel = 0.05;   // ganancia por velocidad de estiramiento (componente dinámico)

    // Activación de realimentación [0..1] a partir del estado del actuador.
    // Solo excitatoria (un actuador solo tira): si no está estirado, no hay realimentación.
    Real activation(Real length, Real velocity) const {
        Real a = k_len * (length - l_rest) + k_vel * velocity;
        if (a < 0.0) return 0.0;
        if (a > 1.0) return 1.0;
        return a;
    }
};

}  // namespace soma::control
