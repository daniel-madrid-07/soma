// SOMA — Test FASE 3 (CPG→articulación): movimiento rítmico EMERGENTE.
//
// El CPG conduce dos actuadores antagonistas (flexor/extensor) en antifase. El
// resultado es una articulación que oscila RÍTMICAMENTE dentro de su rango
// de partes — sin animación, sin keyframes: el ritmo nace de la dinámica nodol,
// la fuerza del modelo no lineal, y el rango de los límites articulares. Es el ladrillo
// de la locomoción (Fase 6). Con el CPG apagado, la articulación se queda quieta.
#include "actuators/spring/spring_actuator.hpp"
#include "control/pattern/cpg.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/constraints/joint_limit.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"
#include "sensors/joints/joint_sense.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using actuators::SpringActuator;
using actuators::AttachPoint;

constexpr Vec3 kGravity{0, 0, -9.80665};

struct Joint {
    RigidBody anchor = RigidBody::make_static(Vec3{0, 0, 0});
    RigidBody bone = RigidBody::make_box(1.0, Vec3{0.5, 0.05, 0.05}, Vec3{0.5, 0, 0});
    physics::BallJoint pin{Vec3{-0.5, 0, 0}, Vec3{0, 0, 0}, 0.2};
    physics::AngleLimit1D limit;   // rango de partes
    SpringActuator flexor, extensor;
    AttachPoint flexor_at, extensor_at;
    sensors::JointAngleSense sense;

    Joint() {
        flexor.f_max = extensor.f_max = 300;
        flexor.l_opt = extensor.l_opt = 0.58;
        flexor_at.origin_local = Vec3{0, 0, 0.5};    flexor_at.insertion_local = Vec3{-0.2, 0, 0};
        extensor_at.origin_local = Vec3{0, 0, -0.5}; extensor_at.insertion_local = Vec3{-0.2, 0, 0};
        limit.lo = -1.0; limit.hi = 1.0;   // rango de movimiento
    }

    void step(Real dt, Real flex_drive, Real ext_drive) {
        auto clamp01 = [](Real x){ return x > 1 ? 1 : (x < 0 ? 0 : x); };
        flexor.activation = clamp01(0.4 * flex_drive);
        extensor.activation = clamp01(0.4 * ext_drive);
        physics::apply_gravity(bone, kGravity);
        actuators::apply_actuator(flexor, flexor_at, anchor, bone);
        actuators::apply_actuator(extensor, extensor_at, anchor, bone);
        bone.integrate_velocity(dt);
        for (int it = 0; it < 12; ++it) {
            physics::solve_ball(bone, anchor, pin, dt);
            physics::solve_angle_limit(bone, limit, dt);
        }
        bone.vel *= 0.997; bone.omega *= 0.997;
        bone.integrate_position(dt);
    }
};

// Corre con impulso tónico dado. Devuelve amplitud pico-a-pico y nº de inversiones
// del sentido del ángulo en los últimos 4 s (medida de ritmo).
static void run(Real tonic, Real& amplitude, int& reversals) {
    Joint j;
    control::MatsuokaCPG cpg;
    cpg.s = tonic;
    Real dt = 1.0 / 1000.0;
    Real amin = 1e9, amax = -1e9, prev = 0, prev_dir = 0;
    reversals = 0;
    for (int i = 0; i < 8000; ++i) {   // 8 s
        cpg.step(dt);
        j.step(dt, cpg.flexor_drive(), cpg.extensor_drive());
        Real ang = j.sense.angle(j.bone);
        if (i >= 4000) {
            amin = std::min(amin, ang);
            amax = std::max(amax, ang);
            Real dir = ang - prev;
            if (prev_dir < 0 && dir > 0) ++reversals;
            if (prev_dir > 0 && dir < 0) ++reversals;
            if (std::fabs(dir) > 1e-5) prev_dir = dir;
        }
        prev = ang;
    }
    amplitude = amax - amin;
}

int main() {
    Real amp_on, amp_off;
    int rev_on, rev_off;
    run(1.0, amp_on, rev_on);
    run(0.0, amp_off, rev_off);

    std::fprintf(stderr, "CPG on:  amplitud=%.3f rad  inversiones=%d\n", amp_on, rev_on);
    std::fprintf(stderr, "CPG off: amplitud=%.3f rad  inversiones=%d\n", amp_off, rev_off);

    assert(amp_on > 0.2);    // oscila con amplitud clara
    assert(rev_on >= 4);     // rítmico: varias inversiones de sentido
    assert(amp_off < 0.05);  // apagado: quieto (asentado contra el rango)

    return 0;
}
