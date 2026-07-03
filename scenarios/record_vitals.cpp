// SOMA — grabador de fisiología: exporta a viewer/vitals.js
//   (1) un bucle presión-volumen del ventrículo + ondas de presión (corazón),
//   (2) una línea temporal de FC, ventilación y fatiga en reposo→esfuerzo→recuperación.
// Todo sale de los modelos de L4 (cardiovascular, metabolismo, respiración).
#include "physio/cardiovascular/heart.hpp"
#include "physio/metabolism.hpp"
#include "physio/respiration.hpp"

#include <cstdio>

using namespace soma;
using soma::math::Real;

int main() {
    FILE* f = std::fopen("viewer/vitals.js", "w");
    if (!f) { std::fprintf(stderr, "no pude abrir viewer/vitals.js\n"); return 1; }
    std::fprintf(f, "window.SOMA_VITALS={\n");

    // --- (1) Corazón: estabiliza y graba ~3 latidos ---
    physio::Circulation heart; heart.hr = 75;
    Real dt = 1.0 / 1000.0;
    for (int i = 0; i < 8000; ++i) heart.step(dt);   // estado estacionario
    std::fprintf(f, "\"cardiac\":{\"hr\":75,\"pv\":[");
    Real beats = 3.0 * heart.cycle_period();
    int n = int(beats / dt), stride = 4;
    bool first = true;
    for (int i = 0; i < n; ++i) {
        heart.step(dt);
        if (i % stride) continue;
        if (!first) std::fprintf(f, ",");
        first = false;
        std::fprintf(f, "[%.1f,%.1f,%.1f]", heart.V_lv, heart.P_lv, heart.P_ao);
    }
    std::fprintf(f, "]},\n");

    // --- (2) Energética: reposo → esfuerzo → recuperación ---
    physio::Metabolism met; physio::Respiration resp;
    struct Phase { Real demand, secs; const char* label; };
    Phase plan[] = {{0.4, 30, "reposo"}, {2.3, 45, "esfuerzo"}, {0.3, 75, "recuperación"}};
    std::fprintf(f, "\"energy\":{\"series\":[");
    bool ef = true; Real t = 0; Real edt = 1.0 / 50.0;
    // marcas de fase
    Real m0 = 30, m1 = 30 + 45;
    for (auto& ph : plan) {
        int steps = int(ph.secs / edt);
        for (int i = 0; i < steps; ++i) {
            met.step(edt, ph.demand);
            resp.step(edt, met.drive, ph.demand / met.aerobic_max);
            t += edt;
            if (i % 10) continue;   // ~5 Hz de muestreo
            if (!ef) std::fprintf(f, ",");
            ef = false;
            std::fprintf(f, "[%.1f,%.0f,%.1f,%.3f,%.3f]",
                         t, met.heart_rate(), resp.ventilation, met.fatigue, met.lactate);
        }
    }
    std::fprintf(f, "],\"marks\":[%.0f,%.0f]}\n", m0, m1);

    std::fprintf(f, "};\n");
    std::fclose(f);
    std::fprintf(stderr, "vitals escritos en viewer/vitals.js\n");
    return 0;
}
