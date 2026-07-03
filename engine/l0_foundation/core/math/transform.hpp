// SOMA — L0 — Transform (SE3). Pose rígida: rotación + traslación.
// Cada hueso tiene una. El punto de inserción de un músculo se transforma con esto.
#pragma once

#include "core/math/quat.hpp"
#include "core/math/vec.hpp"

namespace soma::math {

struct Transform {
    Quat rot = Quat::identity();
    Vec3 pos = Zero3;

    constexpr Transform() = default;
    Transform(Quat r, Vec3 p) : rot(r), pos(p) {}

    static Transform identity() { return {}; }

    // Punto local -> mundo.
    Vec3 apply_point(Vec3 local) const { return rot.rotate(local) + pos; }
    // Dirección local -> mundo (sin traslación).
    Vec3 apply_dir(Vec3 local) const { return rot.rotate(local); }

    // Composición: this ∘ o  (aplica o, luego this).
    Transform operator*(const Transform& o) const {
        return {rot * o.rot, pos + rot.rotate(o.pos)};
    }

    Transform inverse() const {
        Quat ri = rot.conjugate();
        return {ri, ri.rotate(-pos)};
    }
};

}  // namespace soma::math
