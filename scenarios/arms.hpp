// SOMA — brazo físico para la visualización. Un péndulo de hombro (húmero) articulado
// a un anclaje, movido por un par de actuadores no lineal antagonistas bajo control PD (igual
// patrón que la postura de Fase 3). El objetivo del hombro lo marca el CPG en contrafase
// con la pierna homolateral → balanceo de brazos EMERGENTE, por física, no cosmético.
#pragma once

#include "actuators/spring/spring_actuator.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/constraints/joint_limit.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"

#include <initializer_list>

namespace soma::scenario {

using soma::math::Real;
using soma::math::Vec3;
using physics::RigidBody;
using actuators::SpringActuator;
using actuators::AttachPoint;

struct ArmRig {
    RigidBody anchor = RigidBody::make_static(Vec3{0, 0, 0});      // hombro (fijo)
    RigidBody upper  = RigidBody::make_box(0.8, Vec3{0.045, 0.045, 0.16}, Vec3{0, 0, -0.16});
    physics::BallJoint shoulder{Vec3{0, 0, 0.16}, Vec3{0, 0, 0}, 0.2};  // cabeza húmero ↔ hombro
    physics::AngleLimit1D lim;
    SpringActuator front, back;                                        // antagonistas del hombro
    AttachPoint front_at, back_at;

    ArmRig() {
        for (SpringActuator* m : {&front, &back}) { m->f_max = 120; m->l_opt = 0.18; }
        front_at.origin_local = Vec3{0.14, 0, 0.04};  front_at.insertion_local = Vec3{0, 0, -0.06};
        back_at.origin_local  = Vec3{-0.14, 0, 0.04}; back_at.insertion_local  = Vec3{0, 0, -0.06};
        lim.axis = {0,1,0}; lim.ref = {0,0,-1}; lim.local_dir = {0,0,-1}; lim.lo = -1.2; lim.hi = 1.2;
    }

    Real angle() const {
        return physics::signed_angle({0,1,0}, {0,0,-1}, upper.orient.rotate({0,0,-1}));
    }

    void step(Real dt, Real target) {
        Real a = angle(), rate = upper.omega.y;
        Real u = 7.0 * (target - a) - 1.2 * rate;      // PD sobre el ángulo del hombro
        auto c01 = [](Real x){ return x > 1 ? 1 : (x < 0 ? 0 : x); };
        back.activation  = c01((u > 0 ? u : 0) + 0.02);   // sube el ángulo (adelante)
        front.activation = c01((u < 0 ? -u : 0) + 0.02);
        physics::apply_gravity(upper, Vec3{0, 0, -9.80665});
        actuators::apply_actuator(front, front_at, anchor, upper);
        actuators::apply_actuator(back, back_at, anchor, upper);
        upper.integrate_velocity(dt);
        for (int it = 0; it < 12; ++it) {
            physics::solve_ball(upper, anchor, shoulder, dt);
            physics::solve_angle_limit(upper, lim, dt);
        }
        upper.vel *= 0.995; upper.omega *= 0.995;
        upper.integrate_position(dt);
    }
};

}  // namespace soma::scenario
