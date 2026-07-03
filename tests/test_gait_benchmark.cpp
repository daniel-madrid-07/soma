// SOMA — Test (L9): banco de validación de la marcha.
//
// Corre el bípedo caminante, graba las señales y comprueba que las métricas de
// marcha caen en rangos plausibles (no aleatorias): velocidad de avance, cadencia
// de zancada, factor de apoyo (duty) y GRF de pico. Es el arnés que evita que un
// sistema "parezca" funcionar sin serlo.
#include "support/biped.hpp"
#include "tools/validation/gait_benchmark.hpp"

#include <cassert>
#include <cstdio>

using namespace soma;
using soma::math::Real;

int main() {
    scenario::GaitTrace tr;
    Real disp = scenario::simulate(/*walking=*/true, /*secs=*/8.0, &tr);

    tools::GaitMetrics m = tools::gait_metrics(tr.t, tr.torso_x, tr.grf0);
    std::fprintf(stderr,
        "marcha: v=%.3f m/s  cadencia=%.2f Hz  duty=%.2f  GRF_pico=%.0f N  apoyos=%d  disp=%.2f m\n",
        m.speed, m.stride_hz, m.duty_factor, m.peak_grf, m.contacts, disp);

    // Rangos plausibles para este caminante sobre rig (no marcha humana exacta, pero
    // sí coherente: avanza, con zancadas periódicas y apoyo alterno real).
    assert(m.speed > 0.1 && m.speed < 1.5);        // avanza a paso razonable
    assert(m.stride_hz > 0.4 && m.stride_hz < 3.0); // cadencia periódica
    assert(m.duty_factor > 0.2 && m.duty_factor < 0.95); // hay apoyo Y balanceo
    assert(m.peak_grf > 50.0);                      // el pie carga peso de verdad
    assert(m.contacts >= 3);                        // varias zancadas en 8 s

    return 0;
}
