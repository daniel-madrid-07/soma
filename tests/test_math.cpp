// SOMA — test de core/math.
#include "core/math/mat.hpp"
#include "core/math/quat.hpp"
#include "core/math/transform.hpp"
#include "core/math/vec.hpp"

#include <cassert>

using namespace soma::math;

static bool eq(Real a, Real b) { return almost_equal(a, b, 1e-9); }
static bool veq(Vec3 a, Vec3 b) { return eq(a.x, b.x) && eq(a.y, b.y) && eq(a.z, b.z); }

int main() {
    // Vec3
    assert(eq(dot(AxisX, AxisY), 0.0));
    assert(veq(cross(AxisX, AxisY), AxisZ));
    assert(eq(length(Vec3{3, 4, 0}), 5.0));
    assert(veq(normalize(Vec3{0, 5, 0}), AxisY));

    // Mat3: identidad y multiplicación
    Mat3 I = Mat3::identity();
    assert(veq(I * Vec3{1, 2, 3}, Vec3{1, 2, 3}));
    Mat3 D = Mat3::diagonal(2, 3, 4);
    assert(veq(D * Vec3{1, 1, 1}, Vec3{2, 3, 4}));

    // Mat3: inversa (D * D^-1 = I)
    Mat3 Dinv = inverse(D);
    Mat3 prod = D * Dinv;
    assert(veq(prod * Vec3{1, 1, 1}, Vec3{1, 1, 1}));

    // Quat: rotar 90° alrededor de Z lleva X -> Y
    Quat qz = Quat::from_axis_angle(AxisZ, HalfPi);
    assert(veq(qz.rotate(AxisX), AxisY));
    // to_mat3 coincide con rotate
    assert(veq(qz.to_mat3() * AxisX, AxisY));
    // composición: dos rotaciones de 90° = 180°
    Quat q180 = qz * qz;
    assert(veq(q180.rotate(AxisX), -AxisX));

    // Transform: punto en marco desplazado y rotado
    Transform t{qz, Vec3{10, 0, 0}};
    assert(veq(t.apply_point(AxisX), Vec3{10, 1, 0}));  // X rota a Y, luego +10 en X
    // inversa deshace
    Transform ti = t.inverse();
    assert(veq(ti.apply_point(t.apply_point(Vec3{2, 3, 4})), Vec3{2, 3, 4}));

    return 0;
}
