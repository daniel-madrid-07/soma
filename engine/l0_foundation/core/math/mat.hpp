// SOMA — L0 — Mat3. Matriz 3x3 (rotaciones, inercia).
// Almacenada por columnas: col0, col1, col2. m * v = combinación lineal de columnas.
#pragma once

#include "core/math/vec.hpp"

namespace soma::math {

struct Mat3 {
    // Columnas.
    Vec3 c0{1, 0, 0}, c1{0, 1, 0}, c2{0, 0, 1};

    constexpr Mat3() = default;
    constexpr Mat3(Vec3 a, Vec3 b, Vec3 c) : c0(a), c1(b), c2(c) {}

    static constexpr Mat3 identity() { return {}; }

    static constexpr Mat3 diagonal(Real dx, Real dy, Real dz) {
        return {{dx, 0, 0}, {0, dy, 0}, {0, 0, dz}};
    }

    // m * v
    constexpr Vec3 operator*(Vec3 v) const { return c0 * v.x + c1 * v.y + c2 * v.z; }

    // m * n  (composición)
    constexpr Mat3 operator*(const Mat3& n) const {
        return {(*this) * n.c0, (*this) * n.c1, (*this) * n.c2};
    }

    constexpr Mat3 operator*(Real s) const { return {c0 * s, c1 * s, c2 * s}; }
    constexpr Mat3 operator+(const Mat3& n) const { return {c0 + n.c0, c1 + n.c1, c2 + n.c2}; }

    constexpr Mat3 transpose() const {
        return {{c0.x, c1.x, c2.x},
                {c0.y, c1.y, c2.y},
                {c0.z, c1.z, c2.z}};
    }

    constexpr Real det() const { return dot(c0, cross(c1, c2)); }
};

// Inversa de una matriz general 3x3 (regla de Cramer).
inline Mat3 inverse(const Mat3& m) {
    Vec3 r0 = cross(m.c1, m.c2);
    Vec3 r1 = cross(m.c2, m.c0);
    Vec3 r2 = cross(m.c0, m.c1);
    Real d = dot(m.c0, r0);
    Real inv = (d > Eps || d < -Eps) ? 1.0 / d : 0.0;
    // Filas r0,r1,r2 escaladas => como columnas de la inversa hay que transponer.
    Mat3 adjT{r0 * inv, r1 * inv, r2 * inv};  // esto son filas
    return adjT.transpose();
}

}  // namespace soma::math
