// SOMA — Test FASE 6: LOCOMOCIÓN. El objetivo del proyecto.
//
// Ensambla toda la cadena causal (ver tests/support/biped.hpp):
//   INTENCIÓN (usuario) → CPG (ritmo) → control PD → activación → músculos Hill
//   → piernas → PIE empuja el SUELO → reacción (GRF + fricción) → el cuerpo AVANZA.
// Sobre un rig de soporte de peso (altura fija, sin vuelco: como un arnés de marcha
// de laboratorio) para aislar la PROPULSIÓN. Cero animación.
//
// Verifica: con intención de caminar, el cuerpo se DESPLAZA hacia adelante; sin
// intención, se queda quieto.
#include "support/biped.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using soma::math::Real;

int main() {
    Real d_walk = scenario::simulate(/*walking=*/true);
    Real d_still = scenario::simulate(/*walking=*/false);
    std::fprintf(stderr, "desplazamiento: caminando=%.3f m   quieto=%.3f m\n", d_walk, d_still);

    assert(std::fabs(d_still) < 0.05);   // sin intención: no se mueve
    assert(d_walk > 0.3);                // con intención: AVANZA hacia adelante (+X)

    return 0;
}
