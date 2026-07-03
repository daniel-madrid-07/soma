// SOMA — L7 — controlador motor de una articulación (PD sobre actuadores antagonistas).
//
// CLAVE: el controlador NO emite par. Emite ACTIVACIÓN [0..1] para cada actuador.
// La fuerza (y por tanto el par) surge del modelo no lineal. Así respetamos la filosofía:
// el agente ordena "cuánto contraer", no "cuánto par ejercer".
//
// Convención de signos: el EXTENSOR aumenta el ángulo articular; el FLEXOR lo
// disminuye. El PD sobre el error de ángulo reparte esfuerzo entre el par antagonista.
#pragma once

#include "core/math/scalar.hpp"

namespace soma::agent {

using math::Real;

struct JointController {
    Real target = 0.0;      // ángulo deseado (rad) — la "intención" a este nivel
    Real kp = 6.0;          // ganancia proporcional (activación por rad de error)
    Real kd = 0.6;          // ganancia derivativa (amortiguación activa)
    Real cocontraction = 0.02;  // co-contracción basal (rigidez, ambos a la vez)

    struct Output { Real flexor; Real extensor; };

    // Dado el ángulo y su velocidad (de la sensor de articulación), reparte activación.
    Output command(Real angle, Real angle_rate) const {
        Real u = kp * (target - angle) - kd * angle_rate;  // >0 => aumentar ángulo
        Real ext = (u > 0 ? u : 0.0) + cocontraction;
        Real flx = (u < 0 ? -u : 0.0) + cocontraction;
        return {clamp01(flx), clamp01(ext)};
    }

    static Real clamp01(Real x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
};

}  // namespace soma::agent
