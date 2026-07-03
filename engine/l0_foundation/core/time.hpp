// SOMA — L0 — tiempo de simulación. Paso fijo, determinista.
// El bucle real usa un acumulador: consume tiempo en pasos fijos de dt.
// Las bandas de frecuencia (ver sim/scheduler) se definen como divisores de la
// tasa base. Ej: base 1000 Hz; cognición cada 100 pasos => 10 Hz.
#pragma once

#include "core/math/scalar.hpp"

#include <cstdint>

namespace soma::time {

using Seconds = soma::math::Real;

// Reloj de paso fijo. base_hz = frecuencia de la banda más rápida.
class FixedClock {
public:
    explicit FixedClock(Seconds base_hz = 1000.0)
        : dt_(1.0 / base_hz), base_hz_(base_hz) {}

    Seconds dt() const { return dt_; }
    Seconds base_hz() const { return base_hz_; }
    std::uint64_t tick() const { return tick_; }
    Seconds now() const { return static_cast<Seconds>(tick_) * dt_; }

    // Avanza un paso fijo.
    void advance() { ++tick_; }

    // ¿Este tick pertenece a una banda que corre cada 'every' pasos base?
    bool on_band(std::uint64_t every) const { return every > 0 && (tick_ % every) == 0; }

    // Cuántos pasos base equivalen a una frecuencia de banda dada.
    std::uint64_t steps_for_hz(Seconds band_hz) const {
        std::uint64_t n = static_cast<std::uint64_t>(base_hz_ / band_hz + 0.5);
        return n == 0 ? 1 : n;
    }

private:
    Seconds dt_;
    Seconds base_hz_;
    std::uint64_t tick_ = 0;
};

// Acumulador para acoplar tiempo real (render) a pasos fijos (sim).
class Accumulator {
public:
    explicit Accumulator(Seconds dt) : dt_(dt) {}
    // Añade tiempo transcurrido; devuelve cuántos pasos fijos ejecutar.
    int add(Seconds frame_time, int max_steps = 8) {
        acc_ += frame_time;
        int steps = 0;
        while (acc_ >= dt_ && steps < max_steps) { acc_ -= dt_; ++steps; }
        return steps;
    }
    Seconds alpha() const { return acc_ / dt_; }  // interpolación para render

private:
    Seconds dt_;
    Seconds acc_ = 0;
};

}  // namespace soma::time
