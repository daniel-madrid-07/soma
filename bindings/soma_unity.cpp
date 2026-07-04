// SOMA — API C del motor para Unity (u otro renderer). Compila a soma.dll.
// Unity NO simula: solo llama a estas funciones (mandar intención, avanzar, leer
// estado) y dibuja. El agente/física es este C++ (el mismo de los 22 tests).
//
// Estado devuelto por soma_get_state(float out[12]):
//   0 hipL   1 kneeL   2 hipR   3 kneeR      (rad, ángulos articulares)
//   4 worldX 5 worldY  6 heading(rad) 7 bob(m, oscilación vertical del torso)
//   8 speed(m/s) 9 grfTotal(N) 10 hipHeight(m) 11 walking(0/1)
#include "support/biped.hpp"
#include "systems/bellows.hpp"
#include "systems/pump/pump.hpp"

#include "../scenarios/arms.hpp"

#include <cmath>

using namespace soma;
using soma::math::Real;

struct SomaWorld {
    scenario::Biped body;
    control::CoupledOscillators cpg{2};
    agent::WalkIntention intent;
    systems::PumpModel pump;
    systems::Bellows bellows;
    scenario::ArmRig arms[2];   // péndulos de hombro físicos (balanceo emergente)
    Real X = 0, Y = 0, heading = 0, prevx = 0, speed = 0, steer = 0;
    Real breath_phase = 0;   // fase del ciclo del fuelle [0..1)
    Real demand = 0;         // impulso de demanda suavizado [0..1]
    Real grf_frame[2] = {0, 0};  // máximo de GRF por pie durante el último frame

    SomaWorld() {
        body.pin_z = false;  // torso con DOF vertical real: bob físico (test_freetorso)
        for (auto& a : arms)
            a.front.f_max = a.back.f_max = 400;  // hombro con reserva para seguir ~1.6 Hz
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
    // demanda: sube al caminar con esfuerzo, decae en reposo (constante ~3 s)
    Real target = w->intent.walk ? std::min(1.0, 0.3 + 0.7 * (w->intent.effort - 0.5)) : 0.05;
    if (target < 0) target = 0;
    w->grf_frame[0] = w->grf_frame[1] = 0;
    for (int i = 0; i < sub; ++i) {
        if (w->intent.walk) w->cpg.step(dt);
        w->body.step(dt, w->cpg, w->intent.walk);
        // brazos: objetivo del CPG en contrafase con la pierna homolateral
        for (int s = 0; s < 2; ++s) {
            Real tgt = w->intent.walk ? 0.30 * std::cos(w->cpg.phase[s]) : 0.0;
            w->arms[s].step(dt, tgt);
        }
        w->grf_frame[0] = std::max(w->grf_frame[0], w->body.grf[0]);
        w->grf_frame[1] = std::max(w->grf_frame[1], w->body.grf[1]);
        w->demand += (target - w->demand) * (dt / 3.0);
        w->pump.set_drive(w->demand, 0.3 * (1.0 - w->demand));
        w->pump.step(dt);
        w->bellows.step(dt, w->demand, w->demand);
        w->breath_phase += dt * w->bellows.cycle_rate / 60.0;
        if (w->breath_phase >= 1.0) w->breath_phase -= 1.0;
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

// --- Estado completo (v1, 32 floats) ---
//  0..3  hipL kneeL hipR kneeR (rad)      4..7  X Y heading bob
//  8 speed  9 grfL 10 grfR  11 hipHeight  12 walking
// 13..16 activaciones pierna L (flexor/extensor cadera, flexor/extensor rodilla)
// 17..20 activaciones pierna R
// 21 pulso (perfil 0..1 de la fase activa de la bomba)
// 22 presión de salida normalizada (p_out/120)
// 23 respiración (0..1, ciclo del fuelle)   24 tasa del fuelle (ciclos/min)
// 25 contacto pie L (0/1)  26 contacto pie R (0/1)
// 27..29 centro de masa (x,y,z)
// 30 demanda [0..1]  31 hombro L (rad)  32 hombro R (rad)  33..35 reservado
__declspec(dllexport) int soma_full_state_size() { return 36; }

__declspec(dllexport) void soma_get_full_state(void* p, float* out) {
    auto* w = static_cast<SomaWorld*>(p);
    soma_get_state(p, out);          // 0..8 compartidos con la vista compacta
    out[9] = (float)w->grf_frame[0];
    out[10] = (float)w->grf_frame[1];
    out[11] = (float)scenario::kHipH;
    out[12] = w->intent.walk ? 1.0f : 0.0f;
    for (int s = 0; s < 2; ++s)
        for (int i = 0; i < 4; ++i)
            out[13 + 4 * s + i] = (float)w->body.legs[s].m[i].act.activation;
    out[21] = (float)w->pump.e_now;
    out[22] = (float)(w->pump.p_out / 120.0);
    out[23] = (float)(0.5 - 0.5 * std::cos(2.0 * math::Pi * w->breath_phase));
    out[24] = (float)w->bellows.cycle_rate;
    out[25] = w->grf_frame[0] > 5.0 ? 1.0f : 0.0f;
    out[26] = w->grf_frame[1] > 5.0 ? 1.0f : 0.0f;
    // centro de masa: torso 4.0 kg + por pierna muslo 0.6 y pantorrilla 0.5
    {
        math::Vec3 com = w->body.torso.pos * 4.0;
        Real mass = 4.0;
        for (int s = 0; s < 2; ++s) {
            com += w->body.legs[s].thigh.pos * 0.6 + w->body.legs[s].shank.pos * 0.5;
            mass += 1.1;
        }
        com *= (1.0 / mass);
        out[27] = (float)com.x; out[28] = (float)com.y; out[29] = (float)com.z;
    }
    out[30] = (float)w->demand;
    out[31] = (float)w->arms[0].angle();
    out[32] = (float)w->arms[1].angle();
    out[33] = out[34] = out[35] = 0.0f;
}

}  // extern "C"
