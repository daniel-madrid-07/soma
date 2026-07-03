// SOMA — diagnóstico de marcha: imprime rodilla/cadera de ambas piernas en el tiempo.
// Sirve para ver si la marcha es simétrica o cojea (rodilla derecha atascada).
#include "support/biped.hpp"
#include <cmath>
#include <cstdio>

using namespace soma;
using soma::math::Real;

int main() {
    scenario::Biped b;
    control::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * 1.0;
    cpg.offset[1] = math::Pi; cpg.phase[0] = 0; cpg.phase[1] = math::Pi;
    Real dt = 1.0 / 1000.0;
    std::printf("  t     hipL   kneeL    hipR   kneeR   torsoX\n");
    for (int i = 0; i < 6000; ++i) {
        cpg.step(dt); b.step(dt, cpg, true);
        if (i % 500 == 0)
            std::printf("%5.2f  %6.3f %6.3f   %6.3f %6.3f   %6.3f\n",
                i * dt,
                b.hip_angle(b.legs[0]), b.knee_angle(b.legs[0]),
                b.hip_angle(b.legs[1]), b.knee_angle(b.legs[1]),
                b.torso.pos.x);
    }
    return 0;
}
