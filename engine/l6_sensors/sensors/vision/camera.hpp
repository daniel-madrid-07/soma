// SOMA — L6 — el OJO. Óptica de cámara estenopeica (pinhole) sobre la sensor.
//
// CLAVE DE CAUSALIDAD: el cámara recibe la luz de la escena (posiciones 3D del mundo)
// y produce una IMAGEN RETINIANA (coordenadas 2D en la sensor). El agente solo
// verá esta imagen — jamás las posiciones del mundo. Así la percepción es real:
// el agente deduce dónde están las cosas desde lo que "ve", no desde la verdad.
//
// Modelo: eje óptico = +X local del cámara; arriba = +Z; derecha = +Y. Cristalino con
// distancia focal 'focal'; sensor de semi-tamaño 'sensor_half' (define el campo).
#pragma once

#include "core/math/quat.hpp"
#include "core/math/vec.hpp"

namespace soma::sensors {

using math::Quat;
using math::Real;
using math::Vec3;

struct Camera {
    Vec3 pos{0, 0, 0};
    Quat orient = Quat::identity();  // orientación del globo de cámara (actuadores de cámara)
    Real focal = 1.0;                // distancia focal del cristalino
    Real sensor_half = 1.0;          // semi-tamaño de la sensor (campo de visión)
    Real pupil = 0.5;                // apertura (afecta a la luz captada; aquí informativa)

    // Proyección de un punto del mundo sobre la sensor.
    struct Projection { bool visible = false; Real u = 0, v = 0; Real depth = 0; };

    Projection project(Vec3 world_point) const {
        // Punto en el marco del cámara (world → local): la luz entra por la pupila.
        Vec3 d = orient.conjugate().rotate(world_point - pos);
        Projection r;
        if (d.x <= 1e-6) return r;              // detrás del cámara: no llega luz a la sensor
        r.u = focal * d.y / d.x;                // proyección estenopeica
        r.v = focal * d.z / d.x;
        r.depth = d.x;
        r.visible = std::fabs(r.u) <= sensor_half && std::fabs(r.v) <= sensor_half;
        return r;
    }
};

}  // namespace soma::sensors
