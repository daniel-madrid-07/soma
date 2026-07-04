// SOMA — experimento F: torso con DOF vertical real (pin_z=false).
// ¿Se sostiene el torso sobre las piernas al caminar, con bob natural?
#include "support/biped.hpp"

#include <cassert>
#include <cstdio>

using namespace soma;
using math::Real;

int main() {
    scenario::Biped b;
    b.pin_z = false;
    agent::WalkIntention intent; intent.walk = true; intent.effort = 1.0;
    control::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * intent.cadence_hz();
    cpg.offset[1] = math::Pi;
    cpg.phase[0] = 0.0; cpg.phase[1] = math::Pi;

    Real dt = 1.0 / 1000.0;
    Real zmin = 1e9, zmax = -1e9, x0 = b.torso.pos.x;
    Real zmean = 0; long n = 0;
    for (int i = 0; i < 8000; ++i) {
        cpg.step(dt);
        b.step(dt, cpg, true);
        if (i > 2000) {  // regimen estacionario
            Real z = b.torso.pos.z;
            zmin = std::min(zmin, z); zmax = std::max(zmax, z);
            zmean += z; ++n;
        }
    }
    Real dx = b.torso.pos.x - x0;
    zmean /= n;
    std::printf("dx=%.2f m  z=[%.3f..%.3f] mean=%.3f  bob=%.1f mm\n",
                dx, zmin, zmax, zmean, (zmax - zmin) * 1000);

    assert(zmin > 0.80 && "torso colapsa");
    assert(zmax < 1.05 && "torso vuela");
    assert(dx > 2.0 && "no avanza");
    assert((zmax - zmin) < 0.12 && "bob excesivo");
    std::puts("test_freetorso OK");
    return 0;
}
