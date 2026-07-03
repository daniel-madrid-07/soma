// SOMA — L4 — metabolismo energético. El cansancio como CONSECUENCIA física.
//
// La contracción muscular consume ATP. El ATP se repone por tres vías:
//   1) fosfocreatina (PCr): tampón inmediato, rápido pero limitado.
//   2) vía aeróbica (oxidativa): sostenible, pero limitada por el aporte de O2
//      (que depende de la respuesta cardiorrespiratoria).
//   3) vía anaeróbica (glucólisis): cubre el resto, pero produce lactato y FATIGA.
//
// Si la demanda supera la capacidad aeróbica, se tira de PCr y de la vía anaeróbica
// → sube el lactato y la fatiga → la fuerza muscular disponible BAJA. En reposo o
// esfuerzo suave, todo es aeróbico y hay recuperación. Nada de esto está scriptado:
// la fatiga emerge del balance entre demanda y aporte de energía.
#pragma once

#include "core/math/scalar.hpp"

#include <algorithm>

namespace soma::physio {

using math::Real;

struct Metabolism {
    // --- Estado [0..1] salvo lactato ---
    Real pcr = 1.0;       // reserva de fosfocreatina
    Real fatigue = 0.0;   // fatiga acumulada (reduce la fuerza)
    Real lactate = 0.0;   // lactato acumulado
    Real drive = 0.0;     // respuesta cardiorrespiratoria integrada [0..1]

    // --- Parámetros ---
    Real aerobic_rest = 0.4;   // capacidad aeróbica en reposo (ATP/s norm.)
    Real aerobic_max = 2.2;    // capacidad aeróbica a máxima entrega de O2
    Real drive_tau = 3.0;      // constante de tiempo de la respuesta cardiorresp. (s)

    // demand: demanda de ATP (ATP/s normalizado; 1.0 ≈ metabolismo aeróbico basal).
    void step(Real dt, Real demand) {
        // Aporte aeróbico: crece con la respuesta cardiorrespiratoria (más O2).
        Real aerobic_cap = aerobic_rest + (aerobic_max - aerobic_rest) * drive;
        Real aerobic = std::min(demand, aerobic_cap);
        Real deficit = demand - aerobic;

        // PCr cubre el déficit inmediato mientras dura.
        Real pcr_supply = std::min(deficit, pcr * 4.0);
        Real anaerobic = std::max(0.0, deficit - pcr_supply);
        pcr -= pcr_supply * dt;

        // Superávit aeróbico → recuperación (recarga PCr, aclara lactato, baja fatiga).
        Real surplus = std::max(0.0, aerobic_cap - demand);
        pcr += surplus * 0.6 * dt;
        pcr = std::clamp(pcr, 0.0, 1.0);

        lactate += anaerobic * 0.6 * dt - lactate * (0.05 + surplus * 0.3) * dt;
        lactate = std::max(0.0, lactate);

        fatigue += anaerobic * 0.18 * dt - surplus * 0.06 * dt - 0.005 * dt;  // recup. pasiva
        fatigue = std::clamp(fatigue, 0.0, 1.0);

        // Respuesta cardiorrespiratoria: nula en reposo (el metabolismo basal ya se
        // cubre), sube con la demanda por encima del basal (anticipación) y con el
        // lactato (retroalimentación química de los quimiorreceptores).
        Real above_rest = (demand - aerobic_rest) / (aerobic_max - aerobic_rest);
        Real target = std::clamp(above_rest + lactate * 0.4, 0.0, 1.0);
        drive += (target - drive) * (dt / drive_tau);
    }

    // La fatiga reduce la fuerza disponible (hasta un 70 %).
    Real force_capacity() const { return 1.0 - 0.7 * fatigue; }

    // Salidas para el resto de sistemas.
    Real heart_rate() const { return 60.0 + 100.0 * drive; }   // lpm
};

}  // namespace soma::physio
