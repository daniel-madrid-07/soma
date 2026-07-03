// SOMA — Test FASE 8 (energía): el DESGASTE como consecuencia del consumo.
//
// Un actuador trabaja; su demanda de energía alimenta el buffer. Si supera el
// suministro sostenible, se tira de la vía de emergencia → sube el residuo y el
// DESGASTE, y el fuelle y la bomba se aceleran para aportar más. El desgaste REDUCE
// la fuerza del actuador — como consecuencia física, no por script. En reposo, se
// recupera. Se verifica el ciclo reposo → esfuerzo → recuperación.
#include "actuators/spring/spring_actuator.hpp"
#include "systems/energy.hpp"
#include "systems/bellows.hpp"

#include <cassert>
#include <cstdio>

using namespace soma;
using soma::math::Real;

struct Body {
    systems::EnergyModel met;
    systems::Bellows bel;
    actuators::SpringActuator m;
    Body() { m.f_max = 1000; m.l_opt = 0.1; m.length = 0.1; m.velocity = 0; }

    // Avanza 'secs' con una demanda dada; el desgaste realimenta al actuador.
    void run(Real demand, Real secs) {
        Real dt = 1.0 / 100.0;   // 100 Hz basta para la dinámica lenta
        for (int i = 0; i < int(secs / dt); ++i) {
            met.step(dt, demand);
            bel.step(dt, met.drive, demand / met.slow_max);
            m.fatigue = met.fatigue;   // el buffer desgasta al actuador
        }
    }
    Real tension() { return m.tension(); }
};

int main() {
    Body b;
    b.m.activation = 1.0;   // el actuador se activa a tope todo el rato

    // --- Reposo suave: todo sostenible, sin desgaste ---
    b.run(0.4, 60.0);
    Real rest_fatigue = b.met.fatigue, rest_rate = b.met.pump_rate();
    Real rest_flow = b.bel.flow, rest_tension = b.tension();
    std::fprintf(stderr, "reposo:  desgaste=%.2f  rate=%.0f  flujo=%.1f/min  tension=%.0f N\n",
                 rest_fatigue, rest_rate, rest_flow, rest_tension);
    assert(rest_fatigue < 0.10);          // sin desgaste apreciable
    assert(rest_rate < 85);               // tasa de reposo
    assert(rest_tension > 950);           // fuerza plena

    // --- Esfuerzo intenso: demanda > suministro sostenible ---
    b.run(2.35, 35.0);
    Real ex_fatigue = b.met.fatigue, ex_rate = b.met.pump_rate();
    Real ex_flow = b.bel.flow, ex_tension = b.tension(), ex_buffer = b.met.buffer;
    std::fprintf(stderr, "esfuerzo: desgaste=%.2f  rate=%.0f  flujo=%.1f/min  buf=%.2f  res=%.2f  tension=%.0f N\n",
                 ex_fatigue, ex_rate, ex_flow, ex_buffer, b.met.residue, ex_tension);
    assert(ex_fatigue > 0.30);            // se acumula desgaste
    assert(ex_rate > 130);                // tasa alta
    assert(ex_flow > 30);                 // caudal alto
    assert(ex_buffer < 0.5);              // se agota la reserva rápida
    assert(ex_tension < 0.75 * rest_tension);  // LA FUERZA BAJA como consecuencia

    // --- Recuperación en reposo ---
    b.run(0.3, 150.0);
    std::fprintf(stderr, "recuperacion: desgaste=%.2f  rate=%.0f  buf=%.2f  tension=%.0f N\n",
                 b.met.fatigue, b.met.pump_rate(), b.met.buffer, b.tension());
    assert(b.met.fatigue < 0.15);         // el desgaste se disipa
    assert(b.met.buffer > 0.85);          // se recarga la reserva rápida
    assert(b.tension() > 850);            // la fuerza se restablece

    return 0;
}
