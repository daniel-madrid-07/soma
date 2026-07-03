// SOMA — L1 — scheduler multi-tasa, paso fijo, determinista.
//
// Cada sistema declara: nombre, frecuencia de banda (Hz), dependencias (corren antes).
// El scheduler ordena por dependencias (orden topológico) y, en cada paso base,
// ejecuta solo los sistemas cuya banda toca en ese tick.
//
// Acoplamiento:
//   - Eventos transitorios (spikes, contactos) => MessageBus, consumido en el
//     MISMO tick por sistemas de la misma banda; se limpia al final del tick.
//   - Estado que cruza bandas (lento<->rápido) => Blackboard (persiste).
// Regla de causalidad: los sistemas solo se comunican por bus/blackboard.
#pragma once

#include "sim/world.hpp"

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace soma::sim {

struct System {
    std::string name;
    soma::math::Real hz = 1000.0;               // frecuencia de banda
    std::function<void(Context&)> update;
    std::vector<std::string> after;             // dependencias (se ejecutan antes)
};

class Scheduler {
public:
    void add(System s) { systems_.push_back(std::move(s)); dirty_ = true; }

    // Resuelve orden topológico y precalcula pasos por banda. Lanza si hay ciclo.
    void build(const World& world) {
        index_.clear();
        for (std::size_t i = 0; i < systems_.size(); ++i) index_[systems_[i].name] = i;

        order_.clear();
        std::vector<int> state(systems_.size(), 0);  // 0=sin visitar,1=en pila,2=hecho
        for (std::size_t i = 0; i < systems_.size(); ++i) visit(i, state);

        every_.resize(systems_.size());
        for (std::size_t i = 0; i < systems_.size(); ++i)
            every_[i] = world.clock.steps_for_hz(systems_[i].hz);
        dirty_ = false;
    }

    // Avanza un paso base: ejecuta sistemas de banda activa en orden, limpia bus.
    void step(World& world) {
        if (dirty_) build(world);
        for (std::size_t idx : order_) {
            std::uint64_t every = every_[idx];
            if (world.clock.on_band(every)) {
                Context ctx{world, static_cast<soma::math::Real>(every) * world.clock.dt()};
                systems_[idx].update(ctx);
            }
        }
        world.clock.advance();
        world.bus.clear_all();
    }

    // Corre N pasos base.
    void run(World& world, std::uint64_t base_steps) {
        for (std::uint64_t i = 0; i < base_steps; ++i) step(world);
    }

    std::size_t system_count() const { return systems_.size(); }

private:
    void visit(std::size_t i, std::vector<int>& state) {
        if (state[i] == 2) return;
        if (state[i] == 1) throw std::runtime_error("SOMA scheduler: ciclo de dependencias en '" + systems_[i].name + "'");
        state[i] = 1;
        for (const auto& dep : systems_[i].after) {
            auto it = index_.find(dep);
            if (it != index_.end()) visit(it->second, state);
            // dependencia inexistente se ignora (permite declarar sistemas futuros)
        }
        state[i] = 2;
        order_.push_back(i);
    }

    std::vector<System> systems_;
    std::unordered_map<std::string, std::size_t> index_;
    std::vector<std::size_t> order_;
    std::vector<std::uint64_t> every_;
    bool dirty_ = true;
};

}  // namespace soma::sim
