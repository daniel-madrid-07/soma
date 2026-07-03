// SOMA — exporta el movimiento de la física a render/motion.json (para Blender).
// Mismo contenido que record_real (ángulos + pose de mundo + brazos) en JSON puro.
#include "arms.hpp"
#include "brain/navigation/steering.hpp"
#include "sensory/vision/eye.hpp"
#include "support/biped.hpp"

#include <cmath>
#include <cstdio>

using namespace soma;
using soma::math::Real;
using soma::math::Vec3;

int main() {
    scenario::Biped b;
    brain::WalkIntention intent; intent.walk = true; intent.effort = 1.0;
    nervous::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * intent.cadence_hz();
    cpg.offset[1] = math::Pi; cpg.phase[0] = 0; cpg.phase[1] = math::Pi;
    sensory::Eye eye; eye.retina_half = 1.3;
    brain::Steering steer;
    scenario::ArmRig armL, armR;
    Vec3 target{7.0, 4.0, 0.0};

    Real dt = 1.0 / 1000.0, X = 0, Y = 0, theta = 0, prevx = b.torso.pos.x;

    FILE* f = std::fopen("render/motion.json", "w");
    if (!f) { std::fprintf(stderr, "no pude abrir render/motion.json\n"); return 1; }
    std::fprintf(f, "{\"fps\":%g,\"target\":[%.2f,%.2f],\"frames\":[\n", 1.0 / (dt * 33), target.x, target.y);

    bool first = true; int hold = 0;
    for (int i = 0; i < 40000; ++i) {
        cpg.step(dt); b.step(dt, cpg, true);
        armL.step(dt, -0.7 * b.hip_angle(b.legs[0]));
        armR.step(dt, -0.7 * b.hip_angle(b.legs[1]));
        Real dloc = b.torso.pos.x - prevx; prevx = b.torso.pos.x;
        X += dloc * std::cos(theta); Y += dloc * std::sin(theta);
        eye.pos = Vec3{X, Y, 1.5};
        eye.orient = math::Quat::from_axis_angle(Vec3{0, 0, 1}, theta);
        auto r = eye.project(Vec3{target.x, target.y, 1.5});
        if (r.visible) theta += steer.turn_rate(brain::Steering::bearing_from_retina(r.u, eye.focal)) * dt;
        if (std::hypot(target.x - X, target.y - Y) < 0.5) { if (++hold > 300) break; }
        if (i % 33) continue;
        if (!first) std::fprintf(f, ",\n"); first = false;
        std::fprintf(f, "[%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f]",
                     b.hip_angle(b.legs[0]), b.knee_angle(b.legs[0]),
                     b.hip_angle(b.legs[1]), b.knee_angle(b.legs[1]),
                     X, Y, theta, b.torso.pos.z - scenario::kZ0, armL.angle(), armR.angle());
    }
    std::fprintf(f, "\n]}\n");
    std::fclose(f);
    std::fprintf(stderr, "render/motion.json escrito\n");
    return 0;
}
