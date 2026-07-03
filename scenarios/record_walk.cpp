// SOMA — grabador de escena: corre el bípedo caminante y exporta las
// transformaciones de cada segmento a viewer/frames.js para el visor 3D.
// El cuerpo 3D se dibuja a partir de ESTOS datos: lo mueve la física, no una
// animación. Cada frame = pose real de cada hueso calculada por la simulación.
#include "support/biped.hpp"

#include <cstdio>

using namespace soma;
using soma::math::Real;
using soma::math::Vec3;
using soma::math::Quat;

static void put(FILE* f, Vec3 p, Quat q) {
    std::fprintf(f, "[%.4f,%.4f,%.4f,%.5f,%.5f,%.5f,%.5f]",
                 p.x, p.y, p.z, q.w, q.x, q.y, q.z);
}

int main() {
    scenario::Biped b;
    brain::WalkIntention intent; intent.walk = true; intent.effort = 1.0;
    nervous::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * intent.cadence_hz();
    cpg.offset[1] = math::Pi;
    cpg.phase[0] = 0.0; cpg.phase[1] = math::Pi;

    Real dt = 1.0 / 1000.0;
    int steps = 10000, stride = 33;   // 10 s, ~30 fps

    FILE* f = std::fopen("viewer/frames.js", "w");
    if (!f) { std::fprintf(stderr, "no pude abrir viewer/frames.js\n"); return 1; }

    std::fprintf(f, "window.SOMA_FRAMES={\n");
    std::fprintf(f, "\"meta\":{\"dt\":%g,\"fps\":%g,\"title\":\"SOMA — marcha emergente\"},\n",
                 dt * stride, 1.0 / (dt * stride));
    // Segmentos: forma, semiejes/radio (m) y color. El cuerpo visual sobre los huesos.
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
        "],\n");
    std::fprintf(f, "\"frames\":[\n");

    bool first = true;
    for (int i = 0; i < steps; ++i) {
        cpg.step(dt);
        b.step(dt, cpg, true);
        if (i % stride) continue;
        if (!first) std::fprintf(f, ",\n");
        first = false;

        auto& L = b.legs[0];
        auto& R = b.legs[1];
        // Pelvis = base del torso; torso encima; cabeza sobre el torso.
        Vec3 pelvis_p = b.torso.pos + b.torso.orient.rotate(Vec3{0, 0, -0.15});
        Vec3 head_p = b.torso.pos + b.torso.orient.rotate(Vec3{0, 0, 0.33});
        Vec3 footL_p = L.shank.pos + L.shank.orient.rotate(Vec3{0.06, 0, -0.2});
        Vec3 footR_p = R.shank.pos + R.shank.orient.rotate(Vec3{0.06, 0, -0.2});

        std::fprintf(f, "{\"s\":[");
        put(f, pelvis_p, b.torso.orient);       std::fprintf(f, ",");
        put(f, b.torso.pos, b.torso.orient);    std::fprintf(f, ",");
        put(f, head_p, b.torso.orient);         std::fprintf(f, ",");
        put(f, L.thigh.pos, L.thigh.orient);    std::fprintf(f, ",");
        put(f, L.shank.pos, L.shank.orient);    std::fprintf(f, ",");
        put(f, R.thigh.pos, R.thigh.orient);    std::fprintf(f, ",");
        put(f, R.shank.pos, R.shank.orient);    std::fprintf(f, ",");
        put(f, footL_p, L.shank.orient);        std::fprintf(f, ",");
        put(f, footR_p, R.shank.orient);
        std::fprintf(f, "],\"grf\":[%.0f,%.0f]}", b.grf[0], b.grf[1]);
    }
    std::fprintf(f, "\n]};\n");
    std::fclose(f);
    std::fprintf(stderr, "frames escritos en viewer/frames.js\n");
    return 0;
}
