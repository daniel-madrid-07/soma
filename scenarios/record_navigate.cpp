// SOMA — grabador: el cuerpo camina HACIA UN OBJETIVO VISIBLE y se exporta a
// viewer/nav.js para el visor. Une el caminante (Fase 6) con la visión (Fase 7):
// el ojo ve el objetivo, el cerebro gira hacia su posición retiniana, las piernas
// avanzan. La trayectoria curva EMERGE de ver y corregir. Nada scriptado.
#include "brain/navigation/steering.hpp"
#include "sensory/vision/eye.hpp"
#include "support/biped.hpp"

#include <cmath>
#include <cstdio>

using namespace soma;
using soma::math::Real;
using soma::math::Vec3;
using soma::math::Quat;

int main() {
    scenario::Biped b;
    brain::WalkIntention intent; intent.walk = true; intent.effort = 1.0;
    nervous::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * intent.cadence_hz();
    cpg.offset[1] = math::Pi; cpg.phase[0] = 0; cpg.phase[1] = math::Pi;

    sensory::Eye eye; eye.retina_half = 1.3;
    brain::Steering steer;
    Vec3 target{6.0, 3.5, 0.0};    // objetivo en el mundo (x,y)

    Real dt = 1.0 / 1000.0;
    Real X = 0, Y = 0, theta = 0;  // pose del cuerpo en el mundo (guiñada)
    Real prevx = b.torso.pos.x;

    FILE* f = std::fopen("viewer/nav.js", "w");
    if (!f) { std::fprintf(stderr, "no pude abrir viewer/nav.js\n"); return 1; }
    std::fprintf(f, "window.SOMA_NAV={\n");
    std::fprintf(f, "\"meta\":{\"dt\":%g,\"fps\":%g,\"target\":[%.2f,%.2f]},\n",
                 dt * 33, 1.0 / (dt * 33), target.x, target.y);
    std::fprintf(f,
        "\"segments\":["
        "{\"name\":\"pelvis\",\"shape\":\"box\",\"half\":[0.115,0.14,0.10],\"color\":\"#e7dccb\"},"
        "{\"name\":\"torso\",\"shape\":\"box\",\"half\":[0.115,0.135,0.185],\"color\":\"#efe6d6\"},"
        "{\"name\":\"head\",\"shape\":\"sphere\",\"r\":0.115,\"color\":\"#f2ead9\"},"
        "{\"name\":\"thighL\",\"shape\":\"capsule\",\"half\":[0.06,0.06,0.20],\"color\":\"#d98b7a\"},"
        "{\"name\":\"shankL\",\"shape\":\"capsule\",\"half\":[0.05,0.05,0.20],\"color\":\"#cf7b6a\"},"
        "{\"name\":\"thighR\",\"shape\":\"capsule\",\"half\":[0.06,0.06,0.20],\"color\":\"#d98b7a\"},"
        "{\"name\":\"shankR\",\"shape\":\"capsule\",\"half\":[0.05,0.05,0.20],\"color\":\"#cf7b6a\"},"
        "{\"name\":\"footL\",\"shape\":\"box\",\"half\":[0.12,0.055,0.03],\"color\":\"#b9a98f\"},"
        "{\"name\":\"footR\",\"shape\":\"box\",\"half\":[0.12,0.055,0.03],\"color\":\"#b9a98f\"}"
        "],\n\"frames\":[\n");

    auto emit = [&](FILE* f, Vec3 p_sim, Quat q_sim, Real torsoX, Quat Rz) {
        Vec3 rel = p_sim - Vec3{torsoX, 0, 0};       // relativo al torso (quita la cinta)
        Vec3 pw = Vec3{X, Y, 0} + Rz.rotate(rel);    // a mundo: gira por rumbo + traslada
        Quat qw = Rz * q_sim;
        std::fprintf(f, "[%.4f,%.4f,%.4f,%.5f,%.5f,%.5f,%.5f]",
                     pw.x, pw.y, pw.z, qw.w, qw.x, qw.y, qw.z);
    };

    bool first = true; int reached_hold = 0;
    for (int i = 0; i < 30000; ++i) {                // hasta 30 s
        cpg.step(dt);
        b.step(dt, cpg, true);

        // Avance real del caminante → posición en el mundo según el rumbo.
        Real dloc = b.torso.pos.x - prevx; prevx = b.torso.pos.x;
        X += dloc * std::cos(theta);
        Y += dloc * std::sin(theta);

        // Visión: el ojo (en el cuerpo) ve el objetivo y gira hacia su imagen retiniana.
        eye.pos = Vec3{X, Y, 1.5};
        eye.orient = Quat::from_axis_angle(Vec3{0, 0, 1}, theta);
        auto r = eye.project(Vec3{target.x, target.y, 1.5});
        if (r.visible)
            theta += steer.turn_rate(brain::Steering::bearing_from_retina(r.u, eye.focal)) * dt;

        Real dist = std::hypot(target.x - X, target.y - Y);
        if (dist < 0.4) { if (++reached_hold > 400) break; }  // llega y se detiene un poco

        if (i % 33) continue;
        if (!first) std::fprintf(f, ",\n");
        first = false;
        Quat Rz = Quat::from_axis_angle(Vec3{0, 0, 1}, theta);
        Real tx = b.torso.pos.x;
        auto& L = b.legs[0]; auto& R = b.legs[1];
        Vec3 pelvis = b.torso.pos + b.torso.orient.rotate(Vec3{0, 0, -0.15});
        Vec3 head = b.torso.pos + b.torso.orient.rotate(Vec3{0, 0, 0.33});
        Vec3 footL = L.shank.pos + L.shank.orient.rotate(Vec3{0.06, 0, -0.2});
        Vec3 footR = R.shank.pos + R.shank.orient.rotate(Vec3{0.06, 0, -0.2});
        std::fprintf(f, "{\"s\":[");
        emit(f, pelvis, b.torso.orient, tx, Rz);      std::fprintf(f, ",");
        emit(f, b.torso.pos, b.torso.orient, tx, Rz); std::fprintf(f, ",");
        emit(f, head, b.torso.orient, tx, Rz);        std::fprintf(f, ",");
        emit(f, L.thigh.pos, L.thigh.orient, tx, Rz); std::fprintf(f, ",");
        emit(f, L.shank.pos, L.shank.orient, tx, Rz); std::fprintf(f, ",");
        emit(f, R.thigh.pos, R.thigh.orient, tx, Rz); std::fprintf(f, ",");
        emit(f, R.shank.pos, R.shank.orient, tx, Rz); std::fprintf(f, ",");
        emit(f, footL, L.shank.orient, tx, Rz);       std::fprintf(f, ",");
        emit(f, footR, R.shank.orient, tx, Rz);
        std::fprintf(f, "],\"p\":[%.3f,%.3f],\"th\":%.3f}", X, Y, theta);
    }
    std::fprintf(f, "\n]};\n");
    std::fclose(f);
    std::fprintf(stderr, "nav escrito: llega a (%.2f,%.2f), objetivo (%.2f,%.2f)\n",
                 X, Y, target.x, target.y);
    return 0;
}
