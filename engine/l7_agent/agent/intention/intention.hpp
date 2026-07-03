// SOMA — L7 — INTENCIÓN. La única entrada del usuario.
//
// El usuario NO controla actuadores ni articulaciones ni pares. Solo expresa una
// INTENCIÓN ("caminar", "más rápido"). El cuerpo hace el resto: la intención
// enciende el CPG (impulso tónico) y fija la cadencia; el CPG genera el ritmo, los
// actuadores la fuerza, el suelo la reacción, el equilibrio la corrección.
//
//   [usuario pulsa W] → WalkIntention.walk = true → CPG on → ... → el cuerpo avanza
//
// Esto materializa la filosofía del proyecto: se controla la intención, no el cuerpo.
#pragma once

#include "core/math/scalar.hpp"

namespace soma::agent {

using math::Real;

struct WalkIntention {
    bool walk = false;   // lo ÚNICO que fija el usuario
    Real effort = 1.0;   // 0..1 empeño (modula cadencia)

    // Traducción a impulso tónico del CPG (0 = sin ritmo).
    Real cpg_tonic() const { return walk ? 1.0 : 0.0; }
    // Cadencia deseada (Hz) a partir del empeño.
    Real cadence_hz() const { return walk ? (0.8 + 0.8 * effort) : 0.0; }
};

}  // namespace soma::agent
