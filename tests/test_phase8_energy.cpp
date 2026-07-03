// SOMA — Test FASE 8 (energía): el CANSANCIO como consecuencia del consumo.
//
// Un músculo trabaja; su demanda de ATP alimenta el metabolismo. Si supera la
// capacidad aeróbica, se tira de la vía anaeróbica → sube el lactato y la FATIGA,
// y la respiración y el corazón se aceleran para aportar más O2. La fatiga REDUCE
// la fuerza del músculo — como consecuencia física, no por script. En reposo, se
// recupera. Se verifica el ciclo reposo → esfuerzo → recuperación.
#include "anatomy/muscle/hill_muscle.hpp"
#include "physio/metabolism.hpp"
#include "physio/respiration.hpp"

#include <cassert>
#include <cstdio>

using namespace soma;
using soma::math::Real;

struct Body {
    physio::Metabolism met;
    physio::Respiration resp;
    anatomy::HillMuscle m;
    Body() { m.f_max = 1000; m.l_opt = 0.1; m.length = 0.1; m.velocity = 0; }

    // Avanza 'secs' con una demanda dada; la fatiga metabólica realimenta al músculo.
    void run(Real demand, Real secs) {
        Real dt = 1.0 / 100.0;   // 100 Hz basta para fisiología lenta
        for (int i = 0; i < int(secs / dt); ++i) {
            met.step(dt, demand);
            resp.step(dt, met.drive, demand / met.aerobic_max);
            m.fatigue = met.fatigue;   // el metabolismo cansa al músculo
        }
    }
    Real tension() { return m.tension(); }
};

int main() {
    Body b;
    b.m.activation = 1.0;   // el músculo se activa a tope todo el rato

    // --- Reposo suave: todo aeróbico, sin fatiga ---
    b.run(0.4, 60.0);
    Real rest_fatigue = b.met.fatigue, rest_hr = b.met.heart_rate();
    Real rest_vent = b.resp.ventilation, rest_tension = b.tension();
    std::fprintf(stderr, "reposo:  fatiga=%.2f  FC=%.0f  vent=%.1f L/min  tensión=%.0f N\n",
                 rest_fatigue, rest_hr, rest_vent, rest_tension);
    assert(rest_fatigue < 0.10);          // sin fatiga apreciable
    assert(rest_hr < 85);                 // pulso de reposo
    assert(rest_tension > 950);           // fuerza plena

    // --- Esfuerzo intenso: demanda > capacidad aeróbica ---
    b.run(2.35, 35.0);
    Real ex_fatigue = b.met.fatigue, ex_hr = b.met.heart_rate();
    Real ex_vent = b.resp.ventilation, ex_tension = b.tension(), ex_pcr = b.met.pcr;
    std::fprintf(stderr, "esfuerzo: fatiga=%.2f  FC=%.0f  vent=%.1f L/min  PCr=%.2f  lact=%.2f  tensión=%.0f N\n",
                 ex_fatigue, ex_hr, ex_vent, ex_pcr, b.met.lactate, ex_tension);
    assert(ex_fatigue > 0.30);            // se acumula fatiga
    assert(ex_hr > 130);                  // taquicardia
    assert(ex_vent > 30);                 // hiperventilación
    assert(ex_pcr < 0.5);                 // se agota la fosfocreatina
    assert(ex_tension < 0.75 * rest_tension);  // LA FUERZA BAJA como consecuencia

    // --- Recuperación en reposo ---
    b.run(0.3, 150.0);
    std::fprintf(stderr, "recuperación: fatiga=%.2f  FC=%.0f  PCr=%.2f  tensión=%.0f N\n",
                 b.met.fatigue, b.met.heart_rate(), b.met.pcr, b.tension());
    assert(b.met.fatigue < 0.15);         // la fatiga se disipa
    assert(b.met.pcr > 0.85);             // se recarga la fosfocreatina
    assert(b.tension() > 850);            // la fuerza se restablece

    return 0;
}
