// SOMA — L7 — navegación por servovisión (visual servoing).
//
// El cuerpo gira hacia DONDE APARECE el objetivo en la retina. No lee la posición
// del objetivo en el mundo: solo su coordenada retiniana (u). Si el objetivo está a
// la derecha en la retina, gira a la derecha, hasta centrarlo en la fóvea. Combinado
// con el avance del caminante (Fase 6), el cuerpo camina hacia lo que ve.
//
//   ojo (imagen) → u en la retina → error de rumbo = atan2(u, focal) → giro
//
// Es la unión de percepción (Fase 7) y locomoción (Fase 6): conducta dirigida a
// una meta, emergente y causal. La "intención" aquí es la meta visual, no un rumbo.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>
#include <cmath>

namespace soma::brain {

using math::Real;

struct Steering {
    Real gain = 2.2;        // ganancia de giro (rad/s por rad de error)
    Real max_rate = 1.4;    // giro máximo (rad/s)

    // Error de rumbo (rad) a partir de la coordenada retiniana horizontal 'u'.
    // Eje óptico = adelante; u>0 = objetivo a un lado → hay que girar hacia él.
    static Real bearing_from_retina(Real u, Real focal) {
        return std::atan2(u, focal);
    }

    // Tasa de giro (rad/s) para reducir el error de rumbo.
    Real turn_rate(Real bearing_error) const {
        return std::clamp(gain * bearing_error, -max_rate, max_rate);
    }
};

}  // namespace soma::brain
