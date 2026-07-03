// SOMA — Test FASE 8 (cardiovascular): el CORAZÓN late y bombea.
//
// El corazón no es una variable: es una bomba con elastancia variable y válvulas.
// Se verifica contra la FISIOLOGÍA humana:
//   - presión aórtica ~120/80 mmHg,
//   - gasto cardíaco ~5 L/min,
//   - fracción de eyección ~50–65 %,
//   - y que responde al sistema autónomo (simpático → sube el gasto).
// Es el `cardio_benchmark` del ROADMAP: si se desvía del rango, el test falla.
#include "physio/cardiovascular/heart.hpp"

#include <cassert>
#include <cstdio>

using namespace soma;
using soma::math::Real;

static physio::Hemodynamics measure(physio::Circulation& c, Real secs) {
    Real dt = 1.0 / 1000.0;
    // Estabiliza varios ciclos antes de medir.
    for (int i = 0; i < int(6.0 / dt); ++i) c.step(dt);
    physio::Hemodynamics h;
    for (int i = 0; i < int(secs / dt); ++i) { c.step(dt); h.sample(c); }
    return h;
}

int main() {
    // --- Reposo (~75 lpm) ---
    physio::Circulation heart;
    heart.hr = 75;
    physio::Hemodynamics h = measure(heart, 4.0);
    Real co = h.cardiac_output_Lmin(heart.hr);
    std::fprintf(stderr,
        "reposo: %.0f/%.0f mmHg  EDV=%.0f ESV=%.0f SV=%.0f mL  FE=%.0f%%  GC=%.1f L/min\n",
        h.p_systolic, h.p_diastolic, h.edv, h.esv, h.stroke_volume(),
        h.ejection_fraction() * 100, co);

    // Presión arterial en rango humano.
    assert(h.p_systolic > 95 && h.p_systolic < 145);
    assert(h.p_diastolic > 55 && h.p_diastolic < 95);
    // Volúmenes y fracción de eyección fisiológicos.
    assert(h.edv > 90 && h.edv < 160);
    assert(h.ejection_fraction() > 0.45 && h.ejection_fraction() < 0.70);
    // Gasto cardíaco ~5 L/min.
    assert(co > 3.5 && co < 7.0);
    // La sangre lleva oxígeno.
    assert(heart.sat_arterial > 0.9);

    // --- Ejercicio: simpático alto → más frecuencia y contractilidad → más gasto ---
    physio::Circulation ex;
    ex.autonomic(/*sympathetic=*/0.9, /*parasympathetic=*/0.0);
    physio::Hemodynamics he = measure(ex, 4.0);
    Real co_ex = he.cardiac_output_Lmin(ex.hr);
    std::fprintf(stderr, "ejercicio: FC=%.0f lpm  GC=%.1f L/min\n", ex.hr, co_ex);

    assert(ex.hr > 120);           // taquicardia por simpático
    assert(co_ex > co * 1.3);      // el gasto sube claramente con el esfuerzo

    return 0;
}
