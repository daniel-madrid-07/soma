// SOMA — test de random, time, events, log/assert.
#include "core/assert.hpp"
#include "core/events.hpp"
#include "core/random.hpp"
#include "core/time.hpp"

#include <cassert>

using namespace soma;

int main() {
    // --- RNG determinista: misma semilla => misma secuencia (invariante #4) ---
    rng::Random a(1234), b(1234);
    for (int i = 0; i < 1000; ++i) assert(a.next_u64() == b.next_u64());
    rng::Random c(1234), d(9999);
    assert(c.next_u64() != d.next_u64());
    // uniform en rango
    rng::Random u(42);
    for (int i = 0; i < 1000; ++i) {
        double x = u.uniform();
        assert(x >= 0.0 && x < 1.0);
    }

    // --- Time: bandas de frecuencia ---
    time::FixedClock clk(1000.0);       // base 1 kHz
    assert(clk.steps_for_hz(10.0) == 100);   // cognición 10 Hz => cada 100 pasos
    assert(clk.steps_for_hz(1000.0) == 1);
    int cognition_ticks = 0;
    for (int i = 0; i < 1000; ++i) {    // 1000 pasos = 1 s
        if (clk.on_band(100)) ++cognition_ticks;
        clk.advance();
    }
    assert(cognition_ticks == 10);      // 10 Hz durante 1 s

    // --- MessageBus: publicar/consumir, luego limpiar ---
    struct Spike { int neuron_id; double volts; };
    events::MessageBus bus;
    bus.publish(Spike{7, -0.055});
    bus.publish(Spike{9, -0.050});
    assert(bus.messages<Spike>().size() == 2);
    assert(bus.messages<Spike>()[0].neuron_id == 7);
    bus.clear_all();
    assert(bus.messages<Spike>().empty());

    // --- Blackboard: estado compartido tipado ---
    struct CoM { double x, y, z; };
    events::Blackboard bb;
    assert(bb.get<CoM>() == nullptr);
    bb.set(CoM{0, 1.0, 0});
    assert(bb.get<CoM>() != nullptr);
    assert(bb.get<CoM>()->y == 1.0);

    SOMA_ASSERT(1 + 1 == 2);
    SOMA_CAUSALITY(true);  // marcador de "no hago trampa aquí"
    return 0;
}
