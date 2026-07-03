// SOMA — Test FASE 3 (CPG): ritmo locomotor emergente.
//
// Verifica que el oscilador de Matsuoka produce un RITMO estable en antifase sin
// reloj externo, y que se apaga si cesa el impulso tónico. Es el motor rítmico de
// la locomoción (Fase 6): una nodo conducirá al flexor, la otra al extensor.
#include "control/pattern/cpg.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using math::Real;

int main() {
    // --- Con impulso tónico: debe oscilar ---
    control::MatsuokaCPG cpg;
    cpg.s = 1.0;
    Real dt = 1.0 / 1000.0;

    int cycles = 0;
    Real prev = cpg.phase_signal();
    Real max_flex = 0, max_ext = 0;
    Real min_when_flex_high = 1e9;   // valor del extensor cuando el flexor es alto
    for (int i = 0; i < 6000; ++i) {  // 6 s
        cpg.step(dt);
        Real ph = cpg.phase_signal();
        if (prev <= 0 && ph > 0) ++cycles;         // cruces por cero ascendentes
        prev = ph;
        Real f = cpg.flexor_drive(), e = cpg.extensor_drive();
        max_flex = std::max(max_flex, f);
        max_ext = std::max(max_ext, e);
        if (f > 0.3) min_when_flex_high = std::min(min_when_flex_high, e);
    }
    std::fprintf(stderr, "CPG: ciclos=%d  max_flex=%.3f  max_ext=%.3f  ext|flex_alto=%.3f\n",
                 cycles, max_flex, max_ext, min_when_flex_high);

    assert(cycles >= 3);                 // hay ritmo sostenido (≈0.5–3 Hz en 6 s)
    assert(max_flex > 0.1 && max_ext > 0.1);   // ambos medio-centros activos
    assert(min_when_flex_high < max_ext * 0.6); // antifase: si flexor alto, extensor bajo

    // --- Sin impulso tónico (s = 0): el ritmo se extingue ---
    control::MatsuokaCPG off;
    off.s = 0.0;
    Real last_activity = 0;
    for (int i = 0; i < 4000; ++i) {
        off.step(dt);
        if (i > 3000)  // último segundo
            last_activity = std::max(last_activity,
                                     std::max(off.flexor_drive(), off.extensor_drive()));
    }
    std::fprintf(stderr, "CPG apagado: actividad final=%.4f\n", last_activity);
    assert(last_activity < 0.05);        // sin impulso, no hay ritmo

    return 0;
}
