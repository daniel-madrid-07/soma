// SOMA — Test FASE 3 (acoplamiento): coordinación inter-articular por fase.
//
// Dos osciladores acoplados deben BLOQUEARSE a un desfase deseado (aquí 90°),
// partiendo de fases arbitrarias. Es la base de la coordinación cadera–rodilla en
// una zancada: cada articulación mantiene su relación de fase con la líder.
#include "control/pattern/coupled_cpg.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using math::Real;

int main() {
    control::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi;          // 1 Hz
    cpg.offset[1] = math::HalfPi;        // rodilla a 90° de la cadera
    cpg.phase[0] = 0.3; cpg.phase[1] = 2.0;  // fases iniciales arbitrarias

    Real dt = 1.0 / 1000.0;
    int cycles = 0;
    Real prev = std::sin(cpg.phase[0]);
    Real mean_dphi = 0; int samples = 0;
    for (int i = 0; i < 6000; ++i) {     // 6 s
        cpg.step(dt);
        Real s = std::sin(cpg.phase[0]);
        if (prev <= 0 && s > 0) ++cycles;
        prev = s;
        if (i >= 4000) {                 // ventana asentada
            mean_dphi += control::wrap_pi(cpg.phase[1] - cpg.phase[0]);
            ++samples;
        }
    }
    mean_dphi /= samples;
    std::fprintf(stderr, "ciclos=%d  desfase medio=%.3f rad (objetivo %.3f)\n",
                 cycles, mean_dphi, math::HalfPi);

    // Frecuencia ~1 Hz (≈6 ciclos en 6 s).
    assert(cycles >= 5 && cycles <= 7);
    // Bloqueado al desfase deseado de 90° (±0.15 rad).
    assert(std::fabs(mean_dphi - math::HalfPi) < 0.15);

    return 0;
}
