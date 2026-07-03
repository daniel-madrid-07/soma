// SOMA — L4 — sistema cardiovascular. El corazón como BOMBA física, no una variable.
//
// Ventrículo izquierdo modelado por ELASTANCIA VARIABLE en el tiempo:
//   P_vi(t) = E(t) · (V_vi − V0)
// E(t) sube en sístole (contracción) y baja en diástole (relajación). Cuatro
// "válvulas" como diodos (solo dejan pasar en un sentido). Carga arterial = modelo
// de Windkessel (distensibilidad aórtica + resistencia periférica).
//
// Cada latido: se llena (diástole), se contrae, expulsa sangre a la aorta (sístole).
// Produce un bucle presión-volumen y un gasto cardíaco fisiológicos. Responde al
// sistema nervioso autónomo (frecuencia y contractilidad).
//
// Unidades fisiológicas (mmHg, mL, s) por claridad clínica; conversión a SI:
// 1 mmHg = 133.322 Pa, 1 mL = 1e-6 m³. Validado contra rangos humanos en el test.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>
#include <cmath>

namespace soma::physio {

using math::Real;

struct Circulation {
    // --- Parámetros del ventrículo ---
    Real E_max = 2.3;     // elastancia sistólica (mmHg/mL) — contractilidad
    Real E_min = 0.06;    // elastancia diastólica (mmHg/mL) — distensibilidad
    Real V0 = 10.0;       // volumen no tensionado (mL)
    Real T_sys = 0.30;    // duración de la sístole (s)

    // --- Válvulas (resistencias) y carga (Windkessel) ---
    Real R_mitral = 0.01; // resistencia de llenado (mmHg·s/mL)
    Real R_aortic = 0.008;// resistencia de eyección
    Real R_sys = 1.05;    // resistencia vascular sistémica
    Real C_ao = 1.6;      // distensibilidad aórtica (mL/mmHg)
    Real P_fill = 8.0;    // presión de llenado (aurícula/venas) (mmHg)
    Real P_ven = 4.0;     // presión venosa central (mmHg)

    Real hr = 75.0;       // frecuencia cardíaca (latidos/min)

    // --- Estado ---
    Real V_lv = 120.0;    // volumen del ventrículo izquierdo (mL)
    Real P_ao = 80.0;     // presión aórtica (mmHg)
    Real t_cycle = 0.0;   // tiempo dentro del ciclo (s)

    // --- Salidas del último paso ---
    Real P_lv = 0, Q_mitral = 0, Q_aortic = 0, Q_sys = 0, e_act = 0;

    // --- Sangre (oxigenación; los pulmones la reponen en L4 respiratorio) ---
    Real sat_arterial = 0.98;   // saturación de O2 arterial

    Real cycle_period() const { return 60.0 / hr; }

    // La sístole se acorta con la frecuencia (relación de Weissler): a más pulso,
    // relativamente más diástole para poder llenarse. Referencia: T_sys a 75 lpm.
    Real sys_duration() const { return T_sys * (cycle_period() / 0.8); }

    // Función de activación del ciclo cardíaco: sin² durante la sístole, 0 en diástole.
    Real activation(Real tc) const {
        Real Ts = sys_duration();
        if (tc < Ts) { Real s = std::sin(math::Pi * tc / Ts); return s * s; }
        return 0.0;
    }

    // Autónomo: el simpático sube frecuencia y contractilidad; el parasimpático baja.
    void autonomic(Real sympathetic /*0..1*/, Real parasympathetic /*0..1*/) {
        hr = 60.0 + 90.0 * sympathetic - 25.0 * parasympathetic;
        E_max = 2.0 + 1.5 * sympathetic;
    }

    void step(Real dt) {
        Real T = cycle_period();
        t_cycle += dt;
        if (t_cycle >= T) t_cycle -= T;

        e_act = activation(t_cycle);
        Real E = E_min + (E_max - E_min) * e_act;
        P_lv = E * (V_lv - V0);

        Q_mitral = std::max(0.0, (P_fill - P_lv) / R_mitral);  // llenado (válvula mitral)
        Q_aortic = std::max(0.0, (P_lv - P_ao) / R_aortic);    // eyección (válvula aórtica)
        Q_sys = (P_ao - P_ven) / R_sys;                        // salida periférica (Windkessel)

        V_lv += (Q_mitral - Q_aortic) * dt;
        P_ao += ((Q_aortic - Q_sys) / C_ao) * dt;
    }
};

// Métricas hemodinámicas agregadas sobre un intervalo (para validación).
struct Hemodynamics {
    Real p_systolic = 0, p_diastolic = 1e9;  // presión aórtica (mmHg)
    Real edv = 0, esv = 1e9;                  // volúmenes tele-diastólico/sistólico (mL)
    Real mean_flow = 0;                       // gasto instantáneo medio (mL/s)
    long samples = 0;

    void sample(const Circulation& c) {
        p_systolic = std::max(p_systolic, c.P_ao);
        p_diastolic = std::min(p_diastolic, c.P_ao);
        edv = std::max(edv, c.V_lv);
        esv = std::min(esv, c.V_lv);
        mean_flow += c.Q_aortic;
        ++samples;
    }
    Real stroke_volume() const { return edv - esv; }              // mL
    Real ejection_fraction() const { return edv > 0 ? (edv - esv) / edv : 0; }
    Real cardiac_output_Lmin(Real hr) const { return stroke_volume() * hr / 1000.0; }
};

}  // namespace soma::physio
