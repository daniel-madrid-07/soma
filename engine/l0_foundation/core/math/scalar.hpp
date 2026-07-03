// SOMA — L0 — escalares y helpers matemáticos.
// La librería de matemáticas trabaja con double crudo (geometría).
// Las unidades SI (core/units.hpp) se aplican en las fronteras de API.
#pragma once

#include <cmath>

namespace soma::math {

using Real = double;

constexpr Real Pi      = 3.14159265358979323846;
constexpr Real TwoPi   = 2.0 * Pi;
constexpr Real HalfPi  = 0.5 * Pi;
constexpr Real Eps     = 1e-9;

constexpr Real radians(Real deg) { return deg * (Pi / 180.0); }
constexpr Real degrees(Real rad) { return rad * (180.0 / Pi); }

constexpr Real clamp(Real x, Real lo, Real hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}
constexpr Real lerp(Real a, Real b, Real t) { return a + (b - a) * t; }
constexpr Real sign(Real x) { return x > 0 ? 1.0 : (x < 0 ? -1.0 : 0.0); }
constexpr Real sqr(Real x) { return x * x; }

inline bool almost_equal(Real a, Real b, Real eps = Eps) {
    return std::fabs(a - b) <= eps * (1.0 + std::fabs(a) + std::fabs(b));
}

}  // namespace soma::math
