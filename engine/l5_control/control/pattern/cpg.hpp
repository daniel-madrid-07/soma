// SOMA — L5 — Generador Central de Patrones (CPG).
//
// Oscilador de Matsuoka de medio-centro: dos nodos que se inhiben mutuamente y
// se adaptan (desgaste). Producen una salida RÍTMICA en antifase — el sustrato
// nodol del ritmo locomotor en la capa de control. No hay reloj externo: el ritmo EMERGE
// de la dinámica nodol. Una nodo excita al flexor, la otra al extensor.
//
// Ecuaciones (i = 1,2 ; j = la otra):
//   τr·dx_i/dt = -x_i - a·y_j - b·v_i + s + fb_i
//   τa·dv_i/dt = -v_i + y_i
//   y_i = max(0, x_i)
// s = impulso tónico (enciende/apaga y regula amplitud). fb = realimentación
// sensorial (permite que el realimentación module el ritmo). El ritmo cesa si s = 0.
#pragma once

#include "core/math/scalar.hpp"

namespace soma::control {

using math::Real;

struct MatsuokaCPG {
    // Parámetros (calibrados para oscilación estable ~1–2 Hz).
    Real tau_r = 0.05;   // constante de subida (s)
    Real tau_a = 0.30;   // constante de adaptación (s)
    Real a = 2.5;        // inhibición mutua
    Real b = 2.5;        // fuerza de adaptación
    Real s = 1.0;        // impulso tónico (0 => sin ritmo)

    // Estado (inicio asimétrico para romper la simetría y arrancar el ritmo).
    Real x1 = 0.1, x2 = 0.0, v1 = 0.0, v2 = 0.0;

    static Real relu(Real x) { return x > 0.0 ? x : 0.0; }

    // Avanza dt con realimentación sensorial opcional a cada medio-centro.
    void step(Real dt, Real fb1 = 0.0, Real fb2 = 0.0) {
        Real y1 = relu(x1), y2 = relu(x2);
        Real dx1 = (-x1 - a * y2 - b * v1 + s + fb1) / tau_r;
        Real dx2 = (-x2 - a * y1 - b * v2 + s + fb2) / tau_r;
        Real dv1 = (-v1 + y1) / tau_a;
        Real dv2 = (-v2 + y2) / tau_a;
        x1 += dx1 * dt; x2 += dx2 * dt;
        v1 += dv1 * dt; v2 += dv2 * dt;
    }

    Real flexor_drive() const { return relu(x1); }
    Real extensor_drive() const { return relu(x2); }
    // Señal bipolar útil para diagnóstico: >0 domina flexor, <0 domina extensor.
    Real phase_signal() const { return relu(x1) - relu(x2); }
};

}  // namespace soma::control
