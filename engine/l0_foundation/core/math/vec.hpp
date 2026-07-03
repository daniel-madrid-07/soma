// SOMA — L0 — Vec3 (y Vec2). Vectores geométricos en double.
#pragma once

#include "core/math/scalar.hpp"

#include <cmath>

namespace soma::math {

struct Vec3 {
    Real x = 0, y = 0, z = 0;

    constexpr Vec3() = default;
    constexpr Vec3(Real x_, Real y_, Real z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    constexpr Vec3 operator*(Real s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(Real s) const { return {x / s, y / s, z / s}; }

    Vec3& operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(Vec3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(Real s) { x *= s; y *= s; z *= s; return *this; }

    constexpr Real operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
};

constexpr Vec3 operator*(Real s, Vec3 v) { return v * s; }

constexpr Real dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

constexpr Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

inline Real length(Vec3 v) { return std::sqrt(dot(v, v)); }
constexpr Real length_sq(Vec3 v) { return dot(v, v); }

inline Vec3 normalize(Vec3 v) {
    Real len = length(v);
    return len > Eps ? v / len : Vec3{0, 0, 0};
}

inline Real distance(Vec3 a, Vec3 b) { return length(a - b); }

constexpr Vec3 lerp(Vec3 a, Vec3 b, Real t) { return a + (b - a) * t; }

// Ejes canónicos.
constexpr Vec3 AxisX{1, 0, 0};
constexpr Vec3 AxisY{0, 1, 0};
constexpr Vec3 AxisZ{0, 0, 1};
constexpr Vec3 Zero3{0, 0, 0};

}  // namespace soma::math
