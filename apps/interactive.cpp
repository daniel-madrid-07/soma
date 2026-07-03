// SOMA — app INTERACTIVA nativa. Corre el MOTOR C++ en tiempo real y tú mandas
// las señales de INTENCIÓN con el teclado. No es un clip grabado ni una IA que
// decide: eres tú quien expresa la intención; el cuerpo la ejecuta por la cadena
// causal (intención → CPG → control → músculos Hill → huesos → suelo → avance).
//
// Controles (mantén pulsado):
//   W = intención de caminar     A / D = girar izquierda / derecha
//   SHIFT = más rápido (empeño)  ESPACIO = detener de golpe
//   ESC = salir
//
// Visual: HUD por consola (los gráficos vienen después). El motor es el mismo que
// pasa los 22 tests — aquí solo se conduce en vivo.
#include "support/biped.hpp"

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

using namespace soma;
using soma::math::Real;
using soma::math::Vec3;

static bool down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

int main() {
    scenario::Biped body;
    nervous::CoupledOscillators cpg(2);
    cpg.offset[1] = math::Pi;
    cpg.phase[0] = 0.0; cpg.phase[1] = math::Pi;   // piernas en antifase
    brain::WalkIntention intent;

    const Real dt = 1.0 / 1000.0;      // paso físico del motor (1 kHz)
    const int substeps = 16;            // 16 ms de simulación por frame
    const Real frame = substeps * dt;   // 16 ms reales por frame (~62 fps)

    Real X = 0, Y = 0, heading = 0, prevx = body.torso.pos.x;

    std::printf("\n  SOMA — control por INTENCION (motor C++ en vivo)\n");
    std::printf("  --------------------------------------------------\n");
    std::printf("  W = caminar   A/D = girar   SHIFT = rapido   ESPACIO = parar   ESC = salir\n\n");
    std::fflush(stdout);

    auto tick = std::chrono::steady_clock::now();
    long frames = 0;

    while (true) {
        if (down(VK_ESCAPE)) break;

        // --- TU INTENCION desde el teclado (señales, no control directo) ---
        bool want_walk = down('W') && !down(VK_SPACE);
        intent.walk = want_walk;
        intent.effort = down(VK_SHIFT) ? 1.5 : 1.0;
        Real steer = (down('A') ? 1.0 : 0.0) - (down('D') ? 1.0 : 0.0);
        heading += steer * 1.6 * frame;   // giro del rumbo (rad/s)

        // La intención enciende el CPG y fija su cadencia. Nada más la toca.
        cpg.omega = 2.0 * math::Pi * (intent.walk ? intent.cadence_hz() : 1.0);

        // --- El MOTOR ejecuta: CPG → control → músculos → física ---
        for (int i = 0; i < substeps; ++i) {
            if (intent.walk) cpg.step(dt);
            body.step(dt, cpg, intent.walk);
        }

        // Avance real del caminante → posición en el mundo según el rumbo.
        Real dloc = body.torso.pos.x - prevx; prevx = body.torso.pos.x;
        X += dloc * std::cos(heading);
        Y += dloc * std::sin(heading);
        Real speed = dloc / frame;   // m/s

        // --- HUD en una línea (se refresca en el sitio) ---
        Real hipL = body.hip_angle(body.legs[0]) * 180.0 / math::Pi;
        std::printf("\r  [%s]  empeno=%.1f  vel=%.2f m/s  pos=(%.2f, %.2f)  rumbo=%4.0f  cadera=%+5.0f  GRF=%.0f     ",
                    intent.walk ? "CAMINA" : "quieto", intent.effort, speed,
                    X, Y, heading * 180.0 / math::Pi, hipL, body.grf[0] + body.grf[1]);
        std::fflush(stdout);

        // --- Ritmo de tiempo real ---
        tick += std::chrono::microseconds((long)(frame * 1e6));
        std::this_thread::sleep_until(tick);
        ++frames;
    }

    std::printf("\n\n  Fin. Diste %ld pasos de intencion.\n", frames);
    return 0;
}
