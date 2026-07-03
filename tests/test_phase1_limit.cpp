// SOMA — Test FASE 1 (cierre): límite articular anatómico (rango de movimiento).
//
// Un hueso pende de un pivote y la gravedad lo lleva a colgar (90° desde la
// horizontal). Con un límite anatómico de 45°, el hueso DEBE detenerse en 45° y
// no seguir. Control: sin límite alcanza ~90°. Así se modela que un codo no
// hiperextiende — sin scripting, por restricción física.
#include "physics/constraints/ball_joint.hpp"
#include "physics/constraints/joint_limit.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"

#include <cassert>
#include <cmath>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using physics::BallJoint;
using physics::AngleLimit1D;

constexpr Vec3 kGravity{0, 0, -9.80665};

// Simula un péndulo-hueso pinchado en el origen. Devuelve (theta_max, theta_final).
// Si use_limit, aplica un tope superior a 45°.
static void run(bool use_limit, Real& theta_max, Real& theta_final) {
    RigidBody anchor = RigidBody::make_static(Vec3{0, 0, 0});
    RigidBody bone = RigidBody::make_box(1.0, Vec3{0.5, 0.05, 0.05}, Vec3{0.5, 0, 0});

    BallJoint pin{Vec3{-0.5, 0, 0}, Vec3{0, 0, 0}, 0.2};  // extremo del hueso al origen
    AngleLimit1D limit;
    limit.axis = Vec3{0, 1, 0};
    limit.ref = Vec3{1, 0, 0};
    limit.local_dir = Vec3{1, 0, 0};   // eje largo del hueso
    limit.lo = -0.2;
    limit.hi = 0.7853981634;           // 45°

    Real dt = 1.0 / 1000.0;
    theta_max = 0.0;
    for (int step = 0; step < 4000; ++step) {   // 4 s
        physics::apply_gravity(bone, kGravity);
        bone.integrate_velocity(dt);
        for (int it = 0; it < 12; ++it) {
            physics::solve_ball(bone, anchor, pin, dt);
            if (use_limit) physics::solve_angle_limit(bone, limit, dt);
        }
        bone.integrate_position(dt);

        Vec3 r = bone.orient.rotate(limit.local_dir);
        Real theta = physics::signed_angle(limit.axis, limit.ref, r);
        if (theta > theta_max) theta_max = theta;
    }
    Vec3 r = bone.orient.rotate(limit.local_dir);
    theta_final = physics::signed_angle(limit.axis, limit.ref, r);
}

int main() {
    Real max_free, final_free;
    run(/*use_limit=*/false, max_free, final_free);
    // Sin límite: la gravedad lo hace colgar cerca de 90° (1.5708 rad).
    assert(max_free > 1.4);

    Real max_lim, final_lim;
    run(/*use_limit=*/true, max_lim, final_lim);
    // Con límite a 45°: nunca pasa mucho de 45° y reposa contra el tope.
    assert(max_lim < 0.7853981634 + 0.09);         // < ~50°, sobrepaso mínimo
    assert(final_lim > 0.7853981634 - 0.09);       // reposa cerca de 45°
    assert(final_lim < 0.7853981634 + 0.09);

    return 0;
}
