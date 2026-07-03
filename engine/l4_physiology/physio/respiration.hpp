// SOMA — L4 — sistema respiratorio. Entra aire, sale aire, se intercambia gas.
//
// El diafragma y los intercostales (músculos reales) ventilan los pulmones; en los
// alvéolos el O2 pasa a la sangre y el CO2 sale. La VENTILACIÓN no es fija: los
// quimiorreceptores la suben cuando sube el CO2 (o el impulso metabólico). Así, al
// hacer esfuerzo, la respiración se acelera para aportar más O2 y eliminar más CO2
// — como consecuencia de la demanda, no por script.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>

namespace soma::physio {

using math::Real;

struct Respiration {
    // --- Parámetros ---
    Real vital_capacity = 4.8;    // L (capacidad vital)
    Real tidal_rest = 0.5;        // L por respiración en reposo
    Real rate_rest = 12.0;        // respiraciones/min en reposo

    // --- Estado ---
    Real alveolar_co2 = 40.0;     // mmHg (presión parcial alveolar de CO2)
    Real alveolar_o2 = 100.0;     // mmHg
    Real ventilation = 6.0;       // L/min (volumen minuto)
    Real breathing_rate = 12.0;   // respiraciones/min
    Real blood_o2_sat = 0.98;     // saturación arterial

    // drive: impulso ventilatorio [0..1] (del metabolismo / quimiorreceptores).
    // co2_load: producción relativa de CO2 (≈ demanda metabólica).
    void step(Real dt, Real drive, Real co2_load) {
        // La ventilación sube con el impulso (feedforward) y con el CO2 (feedback).
        Real target_vent = 6.0 + 70.0 * drive;                  // L/min
        ventilation += (target_vent - ventilation) * (dt / 1.5);
        breathing_rate = rate_rest + 24.0 * drive;              // rpm
        Real tidal = std::min(vital_capacity * 0.6, tidal_rest + 1.5 * drive);
        (void)tidal;

        // Balance de CO2: se produce (con el esfuerzo) y se elimina (con la ventilación).
        Real co2_removed = ventilation * 0.05;                  // proporcional al volumen minuto
        alveolar_co2 += (co2_load * 8.0 - co2_removed) * dt;
        alveolar_co2 = std::clamp(alveolar_co2, 20.0, 70.0);

        // El O2 alveolar y la saturación mejoran con la ventilación.
        alveolar_o2 = std::clamp(60.0 + ventilation * 2.0, 60.0, 130.0);
        blood_o2_sat = std::clamp(0.90 + 0.02 * (alveolar_o2 - 90.0) / 20.0, 0.85, 1.0);
    }
};

}  // namespace soma::physio
