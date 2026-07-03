// SOMA — Test FASE 3 (pierna 2 segmentos): coordinación cadera–rodilla por CPG.
//
// Un miembro de dos huesos (muslo + tibia) con dos articulaciones (cadera + rodilla),
// cada una con actuadores antagonistas. Dos osciladores acoplados conducen las
// articulaciones. Se demuestra que el CPG COORDINA ambas articulaciones: con desfase
// 0 se mueven en fase (correlación positiva); con desfase π, en oposición (negativa).
// El patrón es EMERGENTE de la dinámica nodol + física. Cero animación.
#include "actuators/spring/spring_actuator.hpp"
#include "control/pattern/coupled_cpg.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/constraints/joint_limit.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"
#include "sensors/joints/joint_sense.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using actuators::SpringActuator;
using actuators::AttachPoint;

constexpr Vec3 kGravity{0, 0, -9.80665};

struct Act { SpringActuator act; AttachPoint at; RigidBody* a; RigidBody* b; };

struct Leg {
    RigidBody pelvis = RigidBody::make_static(Vec3{0, 0, 0});
    RigidBody thigh = RigidBody::make_box(1.0, Vec3{0.25, 0.04, 0.04}, Vec3{0.25, 0, 0});
    RigidBody shank = RigidBody::make_box(1.0, Vec3{0.25, 0.04, 0.04}, Vec3{0.75, 0, 0});

    physics::BallJoint hip{Vec3{-0.25, 0, 0}, Vec3{0, 0, 0}, 0.2};      // muslo–pelvis
    physics::BallJoint knee{Vec3{-0.25, 0, 0}, Vec3{0.25, 0, 0}, 0.2};  // tibia–muslo
    physics::AngleLimit1D hip_lim, knee_lim;

    std::vector<Act> actuators;
    sensors::JointAngleSense sense;   // eje +Y, ref +X, dir +X

    Leg() {
        hip_lim.lo = -1.3; hip_lim.hi = 1.3;
        knee_lim.lo = -1.3; knee_lim.hi = 1.3;
        // Cadera: flexor (origen arriba) / extensor (origen abajo), insertan en el muslo.
        add(&pelvis, &thigh, {0, 0, 0.4},  {-0.1, 0, 0});   // 0 hip flexor
        add(&pelvis, &thigh, {0, 0, -0.4}, {-0.1, 0, 0});   // 1 hip extensor
        // Rodilla: cruzan de muslo a tibia, offset en +/-Z para brazo de momento.
        add(&thigh, &shank, {0.15, 0, 0.15},  {-0.1, 0, 0}); // 2 knee flexor
        add(&thigh, &shank, {0.15, 0, -0.15}, {-0.1, 0, 0}); // 3 knee extensor
        for (auto& m : actuators) { m.act.f_max = 250; m.act.l_opt = 0.4; }
    }
    void add(RigidBody* a, RigidBody* b, Vec3 o, Vec3 i) {
        Act m; m.a = a; m.b = b; m.at.origin_local = o; m.at.insertion_local = i;
        actuators.push_back(m);
    }
    static Real c01(Real x) { return x > 1 ? 1 : (x < 0 ? 0 : x); }

    // PD sobre un par antagonista: reparte activación (flexor baja el ángulo,
    // extensor lo sube). El CPG marca 'target' rítmicamente; el control lo sigue.
    void pd(int flex_idx, int ext_idx, Real target, Real angle, Real rate) {
        Real kp = 8.0, kd = 1.0, coc = 0.02;
        Real u = kp * (target - angle) - kd * rate;   // >0 => subir ángulo (extensor)
        actuators[ext_idx].act.activation = c01((u > 0 ? u : 0) + coc);
        actuators[flex_idx].act.activation = c01((u < 0 ? -u : 0) + coc);
    }

    // targets: referencia rítmica de cadera y de rodilla (relativa) desde el CPG.
    void step(Real dt, Real hip_target, Real knee_target) {
        Real ah = sense.angle(thigh), rh = thigh.omega.y;
        Real ak = sense.angle(shank) - sense.angle(thigh);
        Real rk = shank.omega.y - thigh.omega.y;
        pd(0, 1, hip_target, ah, rh);      // cadera
        pd(2, 3, knee_target, ak, rk);     // rodilla (ángulo relativo al muslo)
        physics::apply_gravity(thigh, kGravity);
        physics::apply_gravity(shank, kGravity);
        for (auto& m : actuators) actuators::apply_actuator(m.act, m.at, *m.a, *m.b);
        thigh.integrate_velocity(dt);
        shank.integrate_velocity(dt);
        for (int it = 0; it < 15; ++it) {
            physics::solve_ball(thigh, pelvis, hip, dt);
            physics::solve_ball(shank, thigh, knee, dt);
            physics::solve_angle_limit(thigh, hip_lim, dt);
            physics::solve_angle_limit(shank, knee_lim, dt);
        }
        thigh.vel *= 0.997; thigh.omega *= 0.997;
        shank.vel *= 0.997; shank.omega *= 0.997;
        thigh.integrate_position(dt);
        shank.integrate_position(dt);
    }
    Real hip_angle() { return sense.angle(thigh); }
    Real knee_angle() { return sense.angle(shank) - sense.angle(thigh); }  // relativo al muslo
};

// Corre con un desfase rodilla-cadera dado. Devuelve std de cada articulación y la
// correlación de Pearson entre el ángulo de cadera y el de rodilla (últimos 4 s).
static void run(Real offset, Real& std_hip, Real& std_knee, Real& corr) {
    Leg leg;
    control::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi;   // 1 Hz
    cpg.offset[1] = offset;
    cpg.phase[0] = 0.0; cpg.phase[1] = offset;  // arranca en el bloqueo estable
    Real dt = 1.0 / 1000.0;
    const Real amp = 0.5;   // amplitud de la referencia rítmica (rad)
    std::vector<Real> H, K;
    for (int i = 0; i < 8000; ++i) {
        cpg.step(dt);
        Real hip_target = amp * std::sin(cpg.phase[0]);
        Real knee_target = amp * std::sin(cpg.phase[1]);
        leg.step(dt, hip_target, knee_target);
        if (i >= 4000) { H.push_back(leg.hip_angle()); K.push_back(leg.knee_angle()); }
    }
    // Medias.
    Real mh = 0, mk = 0;
    for (size_t i = 0; i < H.size(); ++i) { mh += H[i]; mk += K[i]; }
    mh /= H.size(); mk /= K.size();
    // Std y covarianza.
    Real vh = 0, vk = 0, cov = 0;
    for (size_t i = 0; i < H.size(); ++i) {
        Real dh = H[i] - mh, dk = K[i] - mk;
        vh += dh * dh; vk += dk * dk; cov += dh * dk;
    }
    std_hip = std::sqrt(vh / H.size());
    std_knee = std::sqrt(vk / K.size());
    corr = (vh > 1e-9 && vk > 1e-9) ? cov / std::sqrt(vh * vk) : 0.0;
}

int main() {
    Real sh0, sk0, c0, shp, skp, cp;
    run(0.0, sh0, sk0, c0);        // en fase
    run(math::Pi, shp, skp, cp);   // en oposición

    std::fprintf(stderr, "offset 0:  std_cadera=%.3f std_rodilla=%.3f corr=%.3f\n", sh0, sk0, c0);
    std::fprintf(stderr, "offset pi: std_cadera=%.3f std_rodilla=%.3f corr=%.3f\n", shp, skp, cp);

    // Ambas articulaciones oscilan (hay movimiento rítmico real en las dos).
    assert(sh0 > 0.03 && sk0 > 0.03);
    assert(shp > 0.03 && skp > 0.03);
    // El CPG coordina: en fase => correlación positiva; en oposición => negativa.
    assert(c0 > 0.4);
    assert(cp < -0.2);

    return 0;
}
