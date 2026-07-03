// SOMA — L0 — RNG determinista. Invariante #4: misma semilla => mismo resultado.
// splitmix64 para sembrar, xoshiro256** para generar. Sin std::random_device.
// Usos: ruido nodol, variabilidad de reclutamiento motor, jitter sensorial.
#pragma once

#include "core/math/scalar.hpp"

#include <cstdint>
#include <cmath>

namespace soma::rng {

class Random {
public:
    explicit Random(std::uint64_t seed = 0x9E3779B97F4A7C15ull) { reseed(seed); }

    void reseed(std::uint64_t seed) {
        // splitmix64 llena el estado.
        for (auto& s : s_) {
            seed += 0x9E3779B97F4A7C15ull;
            std::uint64_t z = seed;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            s = z ^ (z >> 31);
        }
    }

    std::uint64_t next_u64() {
        const std::uint64_t result = rotl(s_[1] * 5, 7) * 9;
        const std::uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = rotl(s_[3], 45);
        return result;
    }

    // Uniforme en [0,1).
    soma::math::Real uniform() {
        return (next_u64() >> 11) * (1.0 / 9007199254740992.0);  // 2^53
    }

    soma::math::Real uniform(soma::math::Real lo, soma::math::Real hi) {
        return lo + (hi - lo) * uniform();
    }

    // Normal(0,1) por Box-Muller (cachea el segundo valor).
    soma::math::Real normal() {
        if (has_cached_) { has_cached_ = false; return cached_; }
        soma::math::Real u1 = uniform();
        if (u1 < 1e-300) u1 = 1e-300;
        soma::math::Real u2 = uniform();
        soma::math::Real mag = std::sqrt(-2.0 * std::log(u1));
        cached_ = mag * std::sin(soma::math::TwoPi * u2);
        has_cached_ = true;
        return mag * std::cos(soma::math::TwoPi * u2);
    }

    soma::math::Real normal(soma::math::Real mean, soma::math::Real stddev) {
        return mean + stddev * normal();
    }

private:
    static std::uint64_t rotl(std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

    std::uint64_t s_[4]{};
    soma::math::Real cached_ = 0;
    bool has_cached_ = false;
};

}  // namespace soma::rng
