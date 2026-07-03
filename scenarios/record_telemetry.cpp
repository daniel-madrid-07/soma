// SOMA — grabador de telemetría: exporta a viewer/telemetry.js
//   (1) un lazo presión-volumen de la cámara + ondas de presión (bomba),
//   (2) una línea temporal de tasa, caudal y desgaste en reposo→esfuerzo→recuperación.
// Todo sale de los modelos de L4 (bomba, buffer de energía, fuelle).
#include "systems/pump/pump.hpp"
#include "systems/energy.hpp"
#include "systems/bellows.hpp"

#include <cstdio>

using namespace soma;
using soma::math::Real;

int main() {
    FILE* f = std::fopen("viewer/telemetry.js", "w");
    if (!f) { std::fprintf(stderr, "no pude abrir viewer/telemetry.js\n"); return 1; }
    std::fprintf(f, "window.SOMA_TELEMETRY={\n");

    // --- (1) Bomba: estabiliza y graba ~3 ciclos ---
    systems::PumpModel pump; pump.rate = 75;
    Real dt = 1.0 / 1000.0;
    for (int i = 0; i < 8000; ++i) pump.step(dt);   // estado estacionario
    std::fprintf(f, "\"pump\":{\"rate\":75,\"pv\":[");
    Real cycles = 3.0 * pump.cycle_period();
    int n = int(cycles / dt), stride = 4;
    bool first = true;
    for (int i = 0; i < n; ++i) {
        pump.step(dt);
        if (i % stride) continue;
        if (!first) std::fprintf(f, ",");
        first = false;
        std::fprintf(f, "[%.1f,%.1f,%.1f]", pump.volume, pump.p_chamber, pump.p_out);
    }
    std::fprintf(f, "]},\n");

    // --- (2) Energía: reposo → esfuerzo → recuperación ---
    systems::EnergyModel met; systems::Bellows bel;
    struct Phase { Real demand, secs; const char* label; };
    Phase plan[] = {{0.4, 30, "reposo"}, {2.3, 45, "esfuerzo"}, {0.3, 75, "recuperacion"}};
    std::fprintf(f, "\"energy\":{\"series\":[");
    bool ef = true; Real t = 0; Real edt = 1.0 / 50.0;
    // marcas de fase
    Real m0 = 30, m1 = 30 + 45;
    for (auto& ph : plan) {
        int steps = int(ph.secs / edt);
        for (int i = 0; i < steps; ++i) {
            met.step(edt, ph.demand);
            bel.step(edt, met.drive, ph.demand / met.slow_max);
            t += edt;
            if (i % 10) continue;   // ~5 Hz de muestreo
            if (!ef) std::fprintf(f, ",");
            ef = false;
            std::fprintf(f, "[%.1f,%.0f,%.1f,%.3f,%.3f]",
                         t, met.pump_rate(), bel.flow, met.fatigue, met.residue);
        }
    }
    std::fprintf(f, "],\"marks\":[%.0f,%.0f]}\n", m0, m1);

    std::fprintf(f, "};\n");
    std::fclose(f);
    std::fprintf(stderr, "telemetria escrita en viewer/telemetry.js\n");
    return 0;
}
