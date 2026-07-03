// SOMA — Test FASE 7 (visión + locomoción): CAMINAR HACIA LO QUE SE VE.
//
// Une percepción y marcha. El ojo va montado en el cuerpo; ve un objetivo y el
// cerebro gira hacia donde APARECE en la retina (servovisión). El caminante aporta
// la velocidad de avance validada (Fase 6). Resultado: el cuerpo camina en curva y
// ALCANZA el objetivo — sin leer nunca su posición del mundo. Ciego, no lo alcanza.
#include "brain/navigation/steering.hpp"
#include "sensory/vision/eye.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using soma::math::Real;
using soma::math::Vec3;

// Avanza el cuerpo con rumbo θ; si 'sighted', gira según la retina. Devuelve la
// distancia mínima alcanzada al objetivo y si "lo ve".
static Real navigate(bool sighted, Vec3 target, Real& turned, bool& ever_seen) {
    sensory::Eye eye; eye.retina_half = 1.3;   // campo de visión
    brain::Steering steer;
    Real X = 0, Y = 0, theta = 0;              // parte mirando +X
    Real h = 1.5;                              // altura del ojo
    Real speed = 0.38;                         // m/s — avance del caminante (Fase 6)
    Real dt = 0.02;
    Real min_dist = 1e9; turned = 0; ever_seen = false;
    Real theta0 = theta;

    for (int i = 0; i < 3000; ++i) {           // hasta 60 s
        eye.pos = Vec3{X, Y, h};
        eye.orient = math::Quat::from_axis_angle(Vec3{0, 0, 1}, theta);  // guiñada
        if (sighted) {
            auto r = eye.project(target);
            if (r.visible) {
                ever_seen = true;
                Real err = brain::Steering::bearing_from_retina(r.u, eye.focal);
                theta += steer.turn_rate(err) * dt;   // gira hacia el objetivo visto
            }
        }
        X += speed * std::cos(theta) * dt;
        Y += speed * std::sin(theta) * dt;
        Real d = std::hypot(target.x - X, target.y - Y);
        min_dist = std::min(min_dist, d);
        if (d < 0.35) break;                   // llegó
    }
    turned = std::fabs(theta - theta0);
    return min_dist;
}

int main() {
    Vec3 target{6.0, 3.0, 1.5};   // objetivo adelante y a un lado

    Real turned; bool seen;
    Real d_see = navigate(/*sighted=*/true, target, turned, seen);
    std::fprintf(stderr, "con visión: min_dist=%.2f m  giro=%.2f rad  visto=%d\n",
                 d_see, turned, (int)seen);
    assert(seen);              // detectó el objetivo por la retina
    assert(turned > 0.3);      // giró hacia él (no iba recto)
    assert(d_see < 0.5);       // LO ALCANZA

    Real turned0; bool seen0;
    Real d_blind = navigate(/*sighted=*/false, target, turned0, seen0);
    std::fprintf(stderr, "ciego: min_dist=%.2f m  giro=%.2f rad\n", d_blind, turned0);
    assert(turned0 < 0.01);    // no gira
    assert(d_blind > 2.0);     // no se acerca al objetivo

    return 0;
}
