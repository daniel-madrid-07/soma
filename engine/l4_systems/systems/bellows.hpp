// SOMA — L4 — fuelle regulador de un buffer de gas. Entra gas, sale gas, se mezcla.
//
// Un fuelle bombea gas a través de un buffer; el CAUDAL no es fijo: sube cuando sube
// el nivel de carga del buffer (o el impulso de demanda). Así, al aumentar el
// esfuerzo, el caudal se acelera para reponer la carga y drenar el residuo — como
// consecuencia de la demanda, no por script.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>

namespace soma::systems {

using math::Real;

struct Bellows {
    // --- Parámetros ---
    Real capacity = 4.8;      // capacidad del fuelle (unid. vol.)
    Real stroke_rest = 0.5;   // volumen por ciclo en reposo
    Real rate_rest = 12.0;    // ciclos/min en reposo

    // --- Estado ---
    Real load_level = 40.0;   // nivel de residuo en el buffer (unid.)
    Real charge_level = 100.0;// nivel de carga en el buffer (unid.)
    Real flow = 6.0;          // caudal (unid. vol./min)
    Real cycle_rate = 12.0;   // ciclos/min
    Real charge = 0.98;       // carga útil transportada [0..1]

    // drive: impulso de demanda [0..1]. load: producción relativa de residuo.
    void step(Real dt, Real drive, Real load) {
        // El caudal sube con el impulso (anticipación) y con el residuo (realimentación).
        Real target_flow = 6.0 + 70.0 * drive;                  // unid. vol./min
        flow += (target_flow - flow) * (dt / 1.5);
        cycle_rate = rate_rest + 24.0 * drive;                  // ciclos/min
        Real stroke = std::min(capacity * 0.6, stroke_rest + 1.5 * drive);
        (void)stroke;

        // Balance de residuo: se produce (con el esfuerzo) y se drena (con el caudal).
        Real removed = flow * 0.05;                             // proporcional al caudal
        load_level += (load * 8.0 - removed) * dt;
        load_level = std::clamp(load_level, 20.0, 70.0);

        // La carga del buffer y la carga útil mejoran con el caudal.
        charge_level = std::clamp(60.0 + flow * 2.0, 60.0, 130.0);
        charge = std::clamp(0.90 + 0.02 * (charge_level - 90.0) / 20.0, 0.85, 1.0);
    }
};

}  // namespace soma::systems
