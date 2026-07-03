// SOMA — L0 — Quat. Cuaternión unitario para orientaciones (huesos, cabeza, ojo).
#pragma once

#include "core/math/mat.hpp"
#include "core/math/vec.hpp"

#include <cmath>

namespace soma::math {

struct Quat {
    Real w = 1, x = 0, y = 0, z = 0;  // identidad

    constexpr Quat() = default;
    constexpr Quat(Real w_, Real x_, Real y_, Real z_) : w(w_), x(x_), y(y_), z(z_) {}

    static constexpr Quat identity() { return {}; }

    // Rotación de 'angle' rad alrededor de un eje unitario.
    static Quat from_axis_angle(Vec3 axis, Real angle) {
        Vec3 a = normalize(axis);
        Real h = angle * 0.5;
        Real s = std::sin(h);
        return {std::cos(h), a.x * s, a.y * s, a.z * s};
    }

    // Composición: aplica 'o' primero, luego 'this'.
    constexpr Quat operator*(const Quat& o) const {
        return {w * o.w - x * o.x - y * o.y - z * o.z,
                w * o.x + x * o.w + y * o.z - z * o.y,
                w * o.y - x * o.z + y * o.w + z * o.x,
                w * o.z + x * o.y - y * o.x + z * o.w};
    }

    constexpr Quat conjugate() const { return {w, -x, -y, -z}; }

    Real norm() const { return std::sqrt(w * w + x * x + y * y + z * z); }

    Quat normalized() const {
        Real n = norm();
        return n > Eps ? Quat{w / n, x / n, y / n, z / n} : identity();
    }

    // Rota un vector: v' = v + 2w(u×v) + 2(u×(u×v)), u = (x,y,z).
    Vec3 rotate(Vec3 v) const {
        Vec3 u{x, y, z};
        Vec3 t = cross(u, v) * 2.0;
        return v + t * w + cross(u, t);
    }

    Mat3 to_mat3() const {
        Real xx = x * x, yy = y * y, zz = z * z;
        Real xy = x * y, xz = x * z, yz = y * z;
        Real wx = w * x, wy = w * y, wz = w * z;
        return {{1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy)},
                {2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx)},
                {2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy)}};
    }
};

// Interpolación esférica (para suavizar orientaciones sensoriales, no para animar).
inline Quat slerp(Quat a, Quat b, Real t) {
    Real cosom = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    if (cosom < 0) { b = {-b.w, -b.x, -b.y, -b.z}; cosom = -cosom; }
    if (cosom > 1.0 - 1e-6) {  // casi paralelos: lerp
        return Quat{lerp(a.w, b.w, t), lerp(a.x, b.x, t),
                    lerp(a.y, b.y, t), lerp(a.z, b.z, t)}.normalized();
    }
    Real om = std::acos(cosom);
    Real s = std::sin(om);
    Real sa = std::sin((1 - t) * om) / s;
    Real sb = std::sin(t * om) / s;
    return {sa * a.w + sb * b.w, sa * a.x + sb * b.x,
            sa * a.y + sb * b.y, sa * a.z + sb * b.z};
}

}  // namespace soma::math
