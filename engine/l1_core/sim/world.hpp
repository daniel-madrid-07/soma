// SOMA — L1 — World: contenedor de todo el estado compartido de la simulación.
// Un World = un objecto + su entorno. Los sistemas operan sobre él vía Context.
#pragma once

#include "core/events.hpp"
#include "core/random.hpp"
#include "core/time.hpp"
#include "sim/ecs.hpp"
#include "sim/lod.hpp"

namespace soma::sim {

struct World {
    ecs::Registry reg;
    events::MessageBus bus;
    events::Blackboard bb;
    lod::LodManager lod;
    time::FixedClock clock;
    rng::Random rng;

    explicit World(soma::math::Real base_hz = 1000.0, std::uint64_t seed = 1)
        : clock(base_hz), rng(seed) {}
};

// Contexto que recibe cada sistema en su update.
// dt = paso de LA BANDA del sistema (no siempre el paso base).
struct Context {
    World& world;
    soma::math::Real dt;

    ecs::Registry& reg() { return world.reg; }
    events::MessageBus& bus() { return world.bus; }
    events::Blackboard& bb() { return world.bb; }
    lod::LodManager& lod() { return world.lod; }
    rng::Random& rng() { return world.rng; }
    soma::math::Real time() { return world.clock.now(); }
};

}  // namespace soma::sim
