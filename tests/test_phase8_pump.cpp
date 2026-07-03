// SOMA — Test FASE 8 (bomba): la BOMBA cicla y bombea.
//
// La bomba no es una variable: es una cámara de elastancia variable con válvulas.
// Se verifica contra los rangos de referencia del modelo:
//   - presión de salida ~120/80 (unid.),
//   - throughput ~5 (unid./min),
//   - eficiencia ~50–65 %,
//   - y que responde al mando externo (drive alto → sube el throughput).
// Es el `pump_benchmark` del ROADMAP: si se desvía del rango, el test falla.
#include "systems/pump/pump.hpp"

#include <cassert>
#include <cstdio>

using namespace soma;
using soma::math::Real;

static systems::PumpMetrics measure(systems::PumpModel& c, Real secs) {
    Real dt = 1.0 / 1000.0;
    // Estabiliza varios ciclos antes de medir.
    for (int i = 0; i < int(6.0 / dt); ++i) c.step(dt);
    systems::PumpMetrics h;
    for (int i = 0; i < int(secs / dt); ++i) { c.step(dt); h.sample(c); }
    return h;
}

int main() {
    // --- Reposo (~75 ciclos/min) ---
    systems::PumpModel pump;
    pump.rate = 75;
    systems::PumpMetrics h = measure(pump, 4.0);
    Real tp = h.throughput_Lmin(pump.rate);
    std::fprintf(stderr,
        "reposo: %.0f/%.0f  Vhi=%.0f Vlo=%.0f  ciclo=%.0f  ef=%.0f%%  tp=%.1f/min\n",
        h.p_peak, h.p_min, h.v_hi, h.v_lo, h.per_cycle_volume(),
        h.efficiency() * 100, tp);

    // Presión de salida en el rango del modelo.
    assert(h.p_peak > 95 && h.p_peak < 145);
    assert(h.p_min > 55 && h.p_min < 95);
    // Volúmenes y eficiencia dentro de rango.
    assert(h.v_hi > 90 && h.v_hi < 160);
    assert(h.efficiency() > 0.45 && h.efficiency() < 0.70);
    // Throughput ~5 unid./min.
    assert(tp > 3.5 && tp < 7.0);
    // El escalar transportado va lleno.
    assert(pump.charge > 0.9);

    // --- Carga alta: drive alto → más tasa y contractilidad → más throughput ---
    systems::PumpModel ex;
    ex.set_drive(/*hi=*/0.9, /*lo=*/0.0);
    systems::PumpMetrics he = measure(ex, 4.0);
    Real tp_ex = he.throughput_Lmin(ex.rate);
    std::fprintf(stderr, "carga: rate=%.0f/min  tp=%.1f/min\n", ex.rate, tp_ex);

    assert(ex.rate > 120);          // tasa alta por drive
    assert(tp_ex > tp * 1.3);       // el throughput sube claramente con la carga

    return 0;
}
