// SOMA — L7 — corteza visual. Percepción a partir de la imagen retiniana.
//
// El cerebro recibe SOLO la coordenada retiniana (u, v) y conoce la orientación de
// su propio ojo (por la propiocepción de los músculos oculares) y su distancia
// focal. Con eso RECONSTRUYE la dirección hacia el objeto en el mundo. NUNCA lee la
// posición real del objeto: la percepción emerge de la imagen. Esto es lo que hace
// que "ver" sea real y no un truco de leer coordenadas.
#pragma once

#include "core/math/quat.hpp"
#include "core/math/vec.hpp"

namespace soma::brain {

using math::Quat;
using math::Real;
using math::Vec3;

struct VisualCortex {
    // Dirección (unitaria, en el mundo) hacia el objeto, deducida de la retina.
    // Entradas permitidas: coordenada retiniana, focal y orientación del ojo.
    Vec3 direction_from_retina(Real u, Real v, Real focal, Quat eye_orient) const {
        // En el marco del ojo, el rayo que formó (u,v) es (1, u/f, v/f) (eje óptico +X).
        Vec3 ray_local = math::normalize(Vec3{1.0, u / focal, v / focal});
        return eye_orient.rotate(ray_local);   // a coordenadas del mundo
    }

    // Excentricidad retiniana: 0 = en la fóvea (centro), mayor = periferia.
    Real eccentricity(Real u, Real v) const { return std::sqrt(u * u + v * v); }
};

}  // namespace soma::brain
