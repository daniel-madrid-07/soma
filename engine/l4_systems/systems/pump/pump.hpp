// SOMA — L4 — bomba de fluido. Una BOMBA física periódica, no una variable.
//
// Cámara modelada por ELASTANCIA VARIABLE en el tiempo:
//   P_cam(t) = E(t) · (V − V_slack)
// E(t) sube en la fase activa (impulso) y baja en la fase pasiva (rellenado). Dos
// válvulas unidireccionales (como diodos hidráulicos). La carga aguas abajo es un
// depósito compliante con fuga (modelo R–C: distensibilidad + resistencia de fuga).
//
// Cada ciclo: se rellena (fase pasiva), se comprime, expulsa fluido al depósito
// (fase activa). Produce un lazo presión–volumen y un caudal estables. La tasa y la
// contractilidad responden a una señal de mando externa.
//
// Unidades del dominio (unidad de presión, unidad de volumen, s) por claridad; el
// test valida el comportamiento contra rangos de referencia del modelo.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>
#include <cmath>

namespace soma::systems {

using math::Real;

struct PumpModel {
    // --- Parámetros de la cámara ---
    Real e_hi = 2.3;      // elastancia en fase activa (presión/volumen) — contractilidad
    Real e_lo = 0.06;     // elastancia en fase pasiva — distensibilidad
    Real v_slack = 10.0;  // volumen sin tensión (unid. vol.)
    Real t_active = 0.30; // duración de la fase activa (s)

    // --- Válvulas (resistencias) y carga (depósito R–C) ---
    Real r_in = 0.01;     // resistencia de entrada (rellenado)
    Real r_out = 0.008;   // resistencia de salida (expulsión)
    Real r_load = 1.05;   // resistencia de fuga de la carga
    Real c_load = 1.6;    // distensibilidad del depósito (vol./presión)
    Real p_supply = 8.0;  // presión de suministro aguas arriba
    Real p_return = 4.0;  // presión de retorno aguas abajo

    Real rate = 75.0;     // tasa de ciclos (ciclos/min)

    // --- Estado ---
    Real volume = 120.0;  // volumen de la cámara (unid. vol.)
    Real p_out = 80.0;    // presión del depósito de salida
    Real t = 0.0;         // tiempo dentro del ciclo (s)

    // --- Salidas del último paso ---
    Real p_chamber = 0, q_in = 0, q_out = 0, q_load = 0, e_now = 0;

    // --- Escalar transportado (carga útil normalizada 0..1; la repone otra etapa) ---
    Real charge = 0.98;

    Real cycle_period() const { return 60.0 / rate; }

    // La fase activa se acorta a más tasa (deja tiempo relativo de rellenado).
    // Referencia: t_active a 75 ciclos/min.
    Real active_duration() const { return t_active * (cycle_period() / 0.8); }

    // Perfil de la fase activa: sin² durante la fase activa, 0 en la pasiva.
    Real phase_gain(Real tc) const {
        Real Ta = active_duration();
        if (tc < Ta) { Real s = std::sin(math::Pi * tc / Ta); return s * s; }
        return 0.0;
    }

    // Mando externo: 'hi' sube tasa y contractilidad; 'lo' las baja.
    void set_drive(Real hi /*0..1*/, Real lo /*0..1*/) {
        rate = 60.0 + 90.0 * hi - 25.0 * lo;
        e_hi = 2.0 + 1.5 * hi;
    }

    void step(Real dt) {
        Real T = cycle_period();
        t += dt;
        if (t >= T) t -= T;

        e_now = phase_gain(t);
        Real E = e_lo + (e_hi - e_lo) * e_now;
        p_chamber = E * (volume - v_slack);

        q_in = std::max(0.0, (p_supply - p_chamber) / r_in);   // rellenado (válvula de entrada)
        q_out = std::max(0.0, (p_chamber - p_out) / r_out);    // expulsión (válvula de salida)
        q_load = (p_out - p_return) / r_load;                  // fuga de la carga (R–C)

        volume += (q_in - q_out) * dt;
        p_out += ((q_out - q_load) / c_load) * dt;
    }
};

// Métricas agregadas de la bomba sobre un intervalo (para validación).
struct PumpMetrics {
    Real p_peak = 0, p_min = 1e9;   // presión de salida (unid. presión)
    Real v_hi = 0, v_lo = 1e9;      // volúmenes máx./mín. de cámara (unid. vol.)
    Real mean_flow = 0;             // caudal instantáneo medio (vol./s)
    long samples = 0;

    void sample(const PumpModel& c) {
        p_peak = std::max(p_peak, c.p_out);
        p_min = std::min(p_min, c.p_out);
        v_hi = std::max(v_hi, c.volume);
        v_lo = std::min(v_lo, c.volume);
        mean_flow += c.q_out;
        ++samples;
    }
    Real per_cycle_volume() const { return v_hi - v_lo; }             // vol.
    Real efficiency() const { return v_hi > 0 ? (v_hi - v_lo) / v_hi : 0; }
    Real throughput_Lmin(Real rate) const { return per_cycle_volume() * rate / 1000.0; }
};

}  // namespace soma::systems
