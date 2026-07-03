// SOMA — Test FASE 3: control motor. Los actuadores MANTIENEN la postura.
//
// Una articulación (tipo codo) con dos actuadores ANTAGONISTAS (flexor/extensor).
// Un controlador emite ACTIVACIÓN (no par); la fuerza sale del modelo no lineal.
// Un realimentación de estiramiento añade estabilidad. Se verifica:
//   1. mantiene un ángulo objetivo contra la gravedad,
//   2. alcanza distintos objetivos comandados (posicionamiento activo),
//   3. tras un EMPUJÓN, vuelve solo al objetivo (rechazo de perturbación).
// Todo por el lazo sensor->control->activación->fuerza->movimiento. Cero animación.
#include "actuators/spring/spring_actuator.hpp"
#include "agent/motor/joint_controller.hpp"
#include "control/pattern/feedback.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"
#include "sensors/joints/joint_sense.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using actuators::SpringActuator;
using actuators::AttachPoint;

constexpr Vec3 kGravity{0, 0, -9.80665};

struct Arm {
    RigidBody anchor = RigidBody::make_static(Vec3{0, 0, 0});
    RigidBody bone = RigidBody::make_box(1.0, Vec3{0.5, 0.05, 0.05}, Vec3{0.5, 0, 0});
    physics::BallJoint pin{Vec3{-0.5, 0, 0}, Vec3{0, 0, 0}, 0.2};

    SpringActuator flexor, extensor;
    AttachPoint flexor_at, extensor_at;
    control::FeedbackLoop flexor_reflex, extensor_reflex;
    sensors::JointAngleSense sense;
    agent::JointController ctrl;

    Arm() {
        for (SpringActuator* m : {&flexor, &extensor}) {
            m->f_max = 400; m->l_opt = 0.58; m->v_max = 10;
        }
        flexor_at.origin_local = Vec3{0, 0, 0.5};      // por encima => flexiona (−θ)
        flexor_at.insertion_local = Vec3{-0.2, 0, 0};
        extensor_at.origin_local = Vec3{0, 0, -0.5};   // por debajo => extiende (+θ)
        extensor_at.insertion_local = Vec3{-0.2, 0, 0};
        flexor_reflex.l_rest = extensor_reflex.l_rest = 0.583;
        flexor_reflex.k_len = extensor_reflex.k_len = 1.5;
        ctrl.kp = 6.0; ctrl.kd = 0.9;
    }

    void step(Real dt) {
        Real angle = sense.angle(bone);
        Real rate = sense.rate(bone);
        auto cmd = ctrl.command(angle, rate);
        // Activación = orden voluntaria + realimentación local (clamp a 1).
        flexor.activation = agent::JointController::clamp01(
            cmd.flexor + flexor_reflex.activation(flexor.length, flexor.velocity));
        extensor.activation = agent::JointController::clamp01(
            cmd.extensor + extensor_reflex.activation(extensor.length, extensor.velocity));

        physics::apply_gravity(bone, kGravity);
        actuators::apply_actuator(flexor, flexor_at, anchor, bone);
        actuators::apply_actuator(extensor, extensor_at, anchor, bone);
        bone.integrate_velocity(dt);
        for (int it = 0; it < 12; ++it) physics::solve_ball(bone, anchor, pin, dt);
        bone.vel *= 0.999; bone.omega *= 0.999;
        bone.integrate_position(dt);
    }
};

// Mantiene un objetivo durante 'secs'. Devuelve el ángulo final.
static Real hold(Real target, Real secs = 4.0) {
    Arm arm;
    arm.ctrl.target = target;
    Real dt = 1.0 / 1000.0;
    int n = int(secs / dt);
    for (int i = 0; i < n; ++i) arm.step(dt);
    return arm.sense.angle(arm.bone);
}

int main() {
    // 1) Mantener la horizontal (0 rad) contra la gravedad.
    Real a0 = hold(0.0);
    // 2) Objetivos comandados distintos.
    Real aUp = hold(-0.4);
    Real aDn = hold(+0.4);
    std::fprintf(stderr, "hold(0)=%.3f  hold(-0.4)=%.3f  hold(+0.4)=%.3f\n", a0, aUp, aDn);
    assert(std::fabs(a0 - 0.0) < 0.12);
    assert(std::fabs(aUp - (-0.4)) < 0.12);
    assert(std::fabs(aDn - (+0.4)) < 0.12);

    // 3) Rechazo de perturbación: estabiliza en 0, DESPLAZA la postura 0.35 rad
    //    (un empujón que mueve el miembro) y comprueba que VUELVE solo al objetivo.
    Arm arm;
    arm.ctrl.target = 0.0;
    Real dt = 1.0 / 1000.0;
    for (int i = 0; i < 2000; ++i) arm.step(dt);       // 2 s: estabiliza
    Real before = arm.sense.angle(arm.bone);

    // Rota el hueso 0.35 rad alrededor del pivote (origen). El anclaje sigue fijo.
    math::Quat shove = math::Quat::from_axis_angle(Vec3{0, 1, 0}, 0.35);
    arm.bone.orient = shove * arm.bone.orient;
    arm.bone.pos = shove.rotate(arm.bone.pos);
    arm.bone.vel = math::Zero3; arm.bone.omega = math::Zero3;
    Real displaced = arm.sense.angle(arm.bone);

    for (int i = 0; i < 3000; ++i) arm.step(dt);       // 3 s: recupera
    Real after = arm.sense.angle(arm.bone);
    std::fprintf(stderr, "perturbacion: antes=%.3f  desplazado=%.3f  despues=%.3f\n",
                 before, displaced, after);
    assert(std::fabs(before) < 0.12);        // estaba estable
    assert(std::fabs(displaced) > 0.25);     // el empujón sí lo desplazó
    assert(std::fabs(after) < 0.12);         // volvió solo al objetivo

    return 0;
}
