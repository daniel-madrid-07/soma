// SOMA — L4 — buffer de energía. El desgaste como CONSECUENCIA física.
//
// El trabajo de los actuadores consume energía. La energía se repone por tres vías:
//   1) reserva rápida (buffer): tampón inmediato, veloz pero limitado.
//   2) suministro sostenible (lento): estable, pero limitado por la etapa de aporte
//      (que depende de la respuesta del regulador).
//   3) vía de emergencia: cubre el resto, pero deja RESIDUO y DESGASTE.
//
// Si la demanda supera el suministro sostenible, se tira del buffer y de la vía de
// emergencia → sube el residuo y el desgaste → la fuerza disponible BAJA. En reposo
// o esfuerzo suave todo es sostenible y hay recuperación. Nada scriptado: el desgaste
// emerge del balance entre demanda y aporte.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>

namespace soma::systems {

using math::Real;

struct EnergyModel {
    // --- Estado [0..1] salvo residuo ---
    Real buffer = 1.0;    // reserva rápida
    Real fatigue = 0.0;   // desgaste acumulado (reduce la fuerza)
    Real residue = 0.0;   // residuo acumulado
    Real drive = 0.0;     // respuesta del regulador integrada [0..1]

    // --- Parámetros ---
    Real slow_rest = 0.4;   // suministro sostenible en reposo (energía/s norm.)
    Real slow_max = 2.2;    // suministro sostenible a máximo aporte
    Real drive_tau = 3.0;   // constante de tiempo de la respuesta del regulador (s)

    // demand: demanda de energía (energía/s normalizado; 1.0 ≈ nivel basal).
    void step(Real dt, Real demand) {
        // Aporte sostenible: crece con la respuesta del regulador (más aporte).
        Real slow_cap = slow_rest + (slow_max - slow_rest) * drive;
        Real slow = std::min(demand, slow_cap);
        Real deficit = demand - slow;

        // El buffer cubre el déficit inmediato mientras dura.
        Real buf_supply = std::min(deficit, buffer * 4.0);
        Real emergency = std::max(0.0, deficit - buf_supply);
        buffer -= buf_supply * dt;

        // Superávit sostenible → recuperación (recarga buffer, drena residuo, baja desgaste).
        Real surplus = std::max(0.0, slow_cap - demand);
        buffer += surplus * 0.6 * dt;
        buffer = std::clamp(buffer, 0.0, 1.0);

        residue += emergency * 0.6 * dt - residue * (0.05 + surplus * 0.3) * dt;
        residue = std::max(0.0, residue);

        fatigue += emergency * 0.18 * dt - surplus * 0.06 * dt - 0.005 * dt;  // recup. pasiva
        fatigue = std::clamp(fatigue, 0.0, 1.0);

        // Respuesta del regulador: nula en reposo (el basal ya se cubre), sube con la
        // demanda por encima del basal (anticipación) y con el residuo (realimentación).
        Real above_rest = (demand - slow_rest) / (slow_max - slow_rest);
        Real target = std::clamp(above_rest + residue * 0.4, 0.0, 1.0);
        drive += (target - drive) * (dt / drive_tau);
    }

    // El desgaste reduce la fuerza disponible (hasta un 70 %).
    Real output_capacity() const { return 1.0 - 0.7 * fatigue; }

    // Salida para el resto de sistemas (tasa del regulador de la bomba).
    Real pump_rate() const { return 60.0 + 100.0 * drive; }   // ciclos/min
};

}  // namespace soma::systems
