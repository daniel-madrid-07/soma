// SOMA — test: soma_get_full_state expone estado coherente del motor.
// Compila el binding directamente (sin DLL) y camina 6 s simulados.
#include "../bindings/soma_unity.cpp"

#include <cassert>
#include <cstdio>
#include <cmath>

int main() {
    void* w = soma_create();
    assert(soma_full_state_size() == 36);
    soma_set_intention(w, 1, 1.0f, 0.0f);

    float st[36];
    float pulse_min = 1e9f, pulse_max = -1e9f;
    float breath_min = 1e9f, breath_max = -1e9f;
    int contacts = 0, steps = 0;
    float act_max = 0;
    float armL_min = 1e9f, armL_max = -1e9f;
    for (int i = 0; i < 6 * 60; ++i) {         // 6 s a 60 fps
        soma_step(w, 1.0f / 60.0f);
        soma_get_full_state(w, st);
        pulse_min = std::fmin(pulse_min, st[21]); pulse_max = std::fmax(pulse_max, st[21]);
        breath_min = std::fmin(breath_min, st[23]); breath_max = std::fmax(breath_max, st[23]);
        if (st[25] > 0.5f || st[26] > 0.5f) ++contacts;
        for (int k = 13; k <= 20; ++k) act_max = std::fmax(act_max, st[k]);
        if (i > 120) { armL_min = std::fmin(armL_min, st[31]); armL_max = std::fmax(armL_max, st[31]); }
        ++steps;
    }
    std::printf("X=%.2f speed=%.2f pulse=[%.2f..%.2f] breath=[%.2f..%.2f] "
                "contact%%=%.0f actmax=%.2f com=(%.2f,%.2f,%.2f) demand=%.2f arm=[%.2f..%.2f]\n",
                st[4], st[8], pulse_min, pulse_max, breath_min, breath_max,
                100.0 * contacts / steps, act_max, st[27], st[28], st[29], st[30],
                armL_min, armL_max);
    std::fflush(stdout);

    assert(st[4] > 1.0f);                        // avanzó caminando
    assert(pulse_max > 0.8f && pulse_min < 0.1f); // la bomba pulsa (lazo completo)
    assert(breath_max > 0.6f && breath_min < 0.4f); // el fuelle cicla
    assert(contacts > steps / 2);                // hay contacto de pies casi siempre
    assert(act_max > 0.1f);                      // los actuadores trabajan
    assert(st[29] > 0.5f && st[29] < 1.2f);      // CoM a altura plausible
    assert(armL_max - armL_min > 0.2f);          // los brazos se balancean (rad)
    std::puts("test_full_state OK");
    soma_destroy(w);
    return 0;
}
