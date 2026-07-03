// SOMA — API C del motor para Unity (u otro renderer). Compila a soma.dll.
// Unity NO simula: solo llama a estas funciones (mandar intención, avanzar, leer
// estado) y dibuja. El agente/física es este C++ (el mismo de los 22 tests).
//
// Estado devuelto por soma_get_state(float out[12]):
//   0 hipL   1 kneeL   2 hipR   3 kneeR      (rad, ángulos articulares)
//   4 worldX 5 worldY  6 heading(rad) 7 bob(m, oscilación vertical del torso)
//   8 speed(m/s) 9 grfTotal(N) 10 hipHeight(m) 11 walking(0/1)
#include "support/biped.hpp"

#include <cmath>

using namespace soma;
using soma::math::Real;

struct SomaWorld {
    scenario::Biped body;
    control::CoupledOscillators cpg{2};
    agent::WalkIntention intent;
    Real X = 0, Y = 0, heading = 0, prevx = 0, speed = 0, steer = 0;

    SomaWorld() {
        cpg.offset[1] = math::Pi;
        cpg.phase[0] = 0.0; cpg.phase[1] = math::Pi;
        prevx = body.torso.pos.x;
    }
};

extern "C" {

__declspec(dllexport) void* soma_create() { return new SomaWorld(); }

__declspec(dllexport) void soma_destroy(void* p) { delete static_cast<SomaWorld*>(p); }

// walk: 0/1 · effort: 0.5..1.5 · steer: -1 (derecha) .. +1 (izquierda)
__declspec(dllexport) void soma_set_intention(void* p, int walk, float effort, float steer) {
    auto* w = static_cast<SomaWorld*>(p);
    w->intent.walk = walk != 0;
    w->intent.effort = effort;
    w->steer = steer;
}

// Avanza 'frame' segundos de tiempo real (se subdivide a 1 kHz internamente).
__declspec(dllexport) void soma_step(void* p, float frame) {
    auto* w = static_cast<SomaWorld*>(p);
    const Real dt = 1.0 / 1000.0;
    int sub = (int)std::lround(frame / dt);
    if (sub < 1) sub = 1;
    if (sub > 64) sub = 64;

    w->heading += w->steer * 1.6 * frame;
    w->cpg.omega = 2.0 * math::Pi * (w->intent.walk ? w->intent.cadence_hz() : 1.0);
    for (int i = 0; i < sub; ++i) {
        if (w->intent.walk) w->cpg.step(dt);
        w->body.step(dt, w->cpg, w->intent.walk);
    }
    Real dloc = w->body.torso.pos.x - w->prevx;
    w->prevx = w->body.torso.pos.x;
    w->X += dloc * std::cos(w->heading);
    w->Y += dloc * std::sin(w->heading);
    w->speed = (frame > 1e-6) ? dloc / frame : 0.0;
}

__declspec(dllexport) void soma_get_state(void* p, float* out) {
    auto* w = static_cast<SomaWorld*>(p);
    out[0] = (float)w->body.hip_angle(w->body.legs[0]);
    out[1] = (float)w->body.knee_angle(w->body.legs[0]);
    out[2] = (float)w->body.hip_angle(w->body.legs[1]);
    out[3] = (float)w->body.knee_angle(w->body.legs[1]);
    out[4] = (float)w->X;
    out[5] = (float)w->Y;
    out[6] = (float)w->heading;
    out[7] = (float)(w->body.torso.pos.z - scenario::kZ0);
    out[8] = (float)w->speed;
    out[9] = (float)(w->body.grf[0] + w->body.grf[1]);
    out[10] = (float)scenario::kHipH;
    out[11] = w->intent.walk ? 1.0f : 0.0f;
}

}  // extern "C"
