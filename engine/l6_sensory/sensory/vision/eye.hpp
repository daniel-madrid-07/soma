// SOMA — L6 — el OJO. Óptica de cámara estenopeica (pinhole) sobre la retina.
//
// CLAVE DE CAUSALIDAD: el ojo recibe la luz de la escena (posiciones 3D del mundo)
// y produce una IMAGEN RETINIANA (coordenadas 2D en la retina). El cerebro solo
// verá esta imagen — jamás las posiciones del mundo. Así la percepción es real:
// el cerebro deduce dónde están las cosas desde lo que "ve", no desde la verdad.
//
// Modelo: eje óptico = +X local del ojo; arriba = +Z; derecha = +Y. Cristalino con
// distancia focal 'focal'; retina de semi-tamaño 'retina_half' (define el campo).
#pragma once

#include "core/math/quat.hpp"
#include "core/math/vec.hpp"

namespace soma::sensory {

using math::Quat;
using math::Real;
using math::Vec3;

struct Eye {
    Vec3 pos{0, 0, 0};
    Quat orient = Quat::identity();  // orientación del globo ocular (músculos oculares)
    Real focal = 1.0;                // distancia focal del cristalino
    Real retina_half = 1.0;          // semi-tamaño de la retina (campo de visión)
    Real pupil = 0.5;                // apertura (afecta a la luz captada; aquí informativa)

    // Proyección de un punto del mundo sobre la retina.
    struct Retina { bool visible = false; Real u = 0, v = 0; Real depth = 0; };

    Retina project(Vec3 world_point) const {
        // Punto en el marco del ojo (world → local): la luz entra por la pupila.
        Vec3 d = orient.conjugate().rotate(world_point - pos);
        Retina r;
        if (d.x <= 1e-6) return r;              // detrás del ojo: no llega luz a la retina
        r.u = focal * d.y / d.x;                // proyección estenopeica
        r.v = focal * d.z / d.x;
        r.depth = d.x;
        r.visible = std::fabs(r.u) <= retina_half && std::fabs(r.v) <= retina_half;
        return r;
    }
};

}  // namespace soma::sensory
