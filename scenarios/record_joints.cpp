// SOMA — exporta ÁNGULOS ARTICULARES del caminante a viewer/joints.js, para mover
// el esqueleto de un modelo humano rigged (retargeting). El movimiento sigue siendo
// de la física: solo cambia la piel que se dibuja (cajas → malla humana real).
#include "support/biped.hpp"

#include <cstdio>

using namespace soma;
using soma::math::Real;

int main() {
    scenario::Biped b;
    agent::WalkIntention intent; intent.walk = true; intent.effort = 1.0;
    control::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * intent.cadence_hz();
    cpg.offset[1] = math::Pi; cpg.phase[0] = 0; cpg.phase[1] = math::Pi;

    Real dt = 1.0 / 1000.0;
    int steps = 12000, stride = 33;

    FILE* f = std::fopen("viewer/joints.js", "w");
    if (!f) { std::fprintf(stderr, "no pude abrir viewer/joints.js\n"); return 1; }
    std::fprintf(f, "window.SOMA_JOINTS={\n\"meta\":{\"fps\":%g,\"legLen\":0.8},\n\"frames\":[\n",
                 1.0 / (dt * stride));

    bool first = true;
    for (int i = 0; i < steps; ++i) {
        cpg.step(dt);
        b.step(dt, cpg, true);
        if (i % stride) continue;
        if (!first) std::fprintf(f, ",\n");
        first = false;
        Real hL = b.hip_angle(b.legs[0]), kL = b.knee_angle(b.legs[0]);
        Real hR = b.hip_angle(b.legs[1]), kR = b.knee_angle(b.legs[1]);
        Real fwd = b.torso.pos.x;                 // avance (m)
        Real bob = b.torso.pos.z - scenario::kZ0; // oscilación vertical del torso (m)
        // hipL, kneeL, hipR, kneeR (rad), avance, bob
        std::fprintf(f, "[%.4f,%.4f,%.4f,%.4f,%.4f,%.4f]", hL, kL, hR, kR, fwd, bob);
    }
    std::fprintf(f, "\n]};\n");
    std::fclose(f);
    std::fprintf(stderr, "joints escritos\n");
    return 0;
}
