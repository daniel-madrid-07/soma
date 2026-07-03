// SOMA — L7 — control postural (estrategia de tobillo).
//
// El cuerpo erguido es un PÉNDULO INVERTIDO: inestable. La gravedad amplifica
// cualquier inclinación. Mantenerse de pie exige realimentación activa: el
// controlador integra la señal vestibular (inclinación + velocidad) y reparte
// ACTIVACIÓN entre los músculos del tobillo (posterior/anterior) para corregir.
// No emite par: la fuerza sale del modelo Hill.
//
// Estabilidad: para un péndulo invertido, la ganancia proporcional efectiva debe
// superar el término desestabilizador de la gravedad (m·g·l). Si es insuficiente,
// el cuerpo cae — como debe ser.
#pragma once

#include "core/math/scalar.hpp"

namespace soma::brain {

using math::Real;

struct BalanceController {
    Real kp = 5.0;   // respuesta a la inclinación
    Real kd = 1.2;   // respuesta a la velocidad de inclinación (amortiguación)

    struct Output { Real posterior; Real anterior; };

    // tilt: + = inclinado hacia adelante (+X). rate: velocidad de cabeceo.
    Output command(Real tilt, Real rate) const {
        Real u = kp * tilt + kd * rate;   // >0 => cayendo adelante => tirar atrás
        Real post = u > 0 ? u : 0.0;      // músculo posterior (recupera de caída adelante)
        Real ant = u < 0 ? -u : 0.0;      // músculo anterior (recupera de caída atrás)
        return {clamp01(post), clamp01(ant)};
    }

    static Real clamp01(Real x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
};

}  // namespace soma::brain
