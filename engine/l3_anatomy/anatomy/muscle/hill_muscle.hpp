// SOMA — L3 — músculo tipo Hill. El actuador biológico que MUEVE los huesos.
//
// Fuerza = activación · Fmax · fL(l) · fV(v)  +  fuerza pasiva(l).   (tensión ≥ 0)
//   - fL: curva fuerza-longitud (Gaussiana centrada en la longitud óptima).
//   - fV: curva fuerza-velocidad de Hill (menos fuerza al acortar rápido; meseta
//         excéntrica al alargar).
//   - pasiva: elasticidad que aparece al estirar más allá de la longitud óptima.
//
// La ACTIVACIÓN [0..1] la fija el sistema nervioso (unión neuromuscular, L5).
// Aquí el músculo solo convierte activación + estado mecánico en FUERZA.
// (Aproximación de tendón rígido; el elemento elástico en serie del tendón llega
//  en anatomy/tendon.)
#pragma once

#include "core/math/scalar.hpp"
#include "core/math/vec.hpp"
#include "physics/rigid/body.hpp"

#include <cmath>

namespace soma::anatomy {

using math::Real;
using math::Vec3;

struct HillMuscle {
    // --- Parámetros (de la Parameter DB por músculo real) ---
    Real f_max = 800.0;   // fuerza isométrica máxima (N)
    Real l_opt = 0.1;     // longitud óptima de fibra (m)
    Real v_max = 10.0;    // vel. máx. de acortamiento (longitudes ópt./s)
    Real width = 0.45;    // ancho de la curva fuerza-longitud
    Real pe_strain = 0.6; // estiramiento (fracción) donde el pasivo iguala a Fmax
    Real a_f = 0.25;      // forma de la hipérbola de Hill (fuerza-velocidad)
    Real f_ecc = 1.5;     // meseta excéntrica (múltiplo de Fmax al alargar)

    // --- Estado (actualizado desde la mecánica cada paso) ---
    Real activation = 0.0;  // [0..1], la fija el sistema nervioso
    Real length = 0.1;      // longitud actual (m)
    Real velocity = 0.0;    // tasa de cambio de longitud (m/s); + = alargando
    Real fatigue = 0.0;     // [0..1], del metabolismo (L4); reduce la fuerza activa

    // Curva fuerza-longitud activa: Gaussiana, pico 1.0 en l = l_opt.
    Real force_length(Real l) const {
        Real x = (l / l_opt - 1.0) / width;
        return std::exp(-x * x);
    }

    // Curva fuerza-velocidad de Hill. v en m/s (+ alargando).
    Real force_velocity(Real v) const {
        Real vn = v / (l_opt * v_max);        // velocidad normalizada
        if (vn < -1.0) vn = -1.0;             // no más rápido que v_max
        if (vn <= 0.0)                        // concéntrico (acortando)
            return (1.0 + vn) / (1.0 - vn / a_f);       // 1 en 0, 0 en vn=-1
        return (f_ecc * vn + a_f) / (vn + a_f);         // excéntrico: 1 -> f_ecc
    }

    // Fuerza pasiva (elástica) al estirar por encima de l_opt. Fracción de Fmax.
    Real force_passive(Real l) const {
        Real ln = l / l_opt;
        if (ln <= 1.0) return 0.0;
        Real x = (ln - 1.0) / pe_strain;
        return x * x;
    }

    // Tensión total en newtons (nunca negativa: un músculo solo tira).
    // La fatiga reduce la parte ACTIVA (la pasiva es elasticidad del tejido, no cansa).
    Real tension() const {
        Real active = activation * force_length(length) * force_velocity(velocity)
                      * (1.0 - fatigue);
        Real f = f_max * (active + force_passive(length));
        return f > 0.0 ? f : 0.0;
    }

    // Demanda de ATP (normalizada) del músculo: sube con la activación y la fuerza.
    // Es lo que el metabolismo (L4) debe cubrir cada instante.
    Real atp_demand() const {
        return activation * (0.3 + 0.7 * force_length(length));
    }
};

// Puntos de anclaje del músculo: origen (en A) e inserción (en B), en marcos locales.
struct MuscleAttachment {
    Vec3 origin_local{0, 0, 0};
    Vec3 insertion_local{0, 0, 0};
};

// Actualiza longitud/velocidad desde las poses actuales y aplica la tensión a los
// huesos. La tensión tira de ambos extremos uno hacia el otro (el músculo acorta).
// Devuelve la fuerza aplicada (N). Cualquiera de los cuerpos puede ser estático.
inline Real apply_muscle(HillMuscle& m, const MuscleAttachment& at,
                         physics::RigidBody& A, physics::RigidBody& B) {
    Vec3 pO = A.pos + A.orient.rotate(at.origin_local);       // origen (mundo)
    Vec3 pI = B.pos + B.orient.rotate(at.insertion_local);    // inserción (mundo)
    Vec3 d = pI - pO;
    Real len = math::length(d);
    Vec3 dir = len > math::Eps ? d / len : Vec3{0, 0, 0};

    Vec3 vO = A.point_velocity(pO);
    Vec3 vI = B.point_velocity(pI);
    m.length = len;
    m.velocity = math::dot(vI - vO, dir);   // + si la distancia crece (alargando)

    Real F = m.tension();
    A.apply_force_at(dir * F, pO);           // origen tirado hacia la inserción
    B.apply_force_at(dir * (-F), pI);        // inserción tirada hacia el origen
    return F;
}

}  // namespace soma::anatomy
