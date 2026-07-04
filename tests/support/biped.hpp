// SOMA — soporte de tests: bípedo caminante (rig de soporte de peso).
// Reutilizado por test_phase6_locomotion y test_gait_benchmark.
// Ensambla intención → CPG → control PD → actuadores no lineal → piernas → contacto suelo.
#pragma once

#include "actuators/spring/spring_actuator.hpp"
#include "agent/intention/intention.hpp"
#include "control/pattern/coupled_cpg.hpp"
#include "physics/collision/ground.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/constraints/joint_limit.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace soma::scenario {

using math::Real;
using math::Vec3;
using physics::RigidBody;
using actuators::SpringActuator;
using actuators::AttachPoint;

constexpr Vec3 kGravity{0, 0, -9.80665};
constexpr Real kZ0 = 0.96;          // altura fija del torso (rig de soporte de peso)
constexpr Real kHipH = kZ0 - 0.15;  // altura de la cadera

struct Act { SpringActuator act; AttachPoint at; RigidBody* a; RigidBody* b; };

struct Leg {
    RigidBody thigh, shank;
    physics::BallJoint hip_j, knee_j;
    physics::AngleLimit1D hip_lim, knee_lim;
    std::vector<Act> m;
};

struct Biped {
    RigidBody torso = RigidBody::make_box(4.0, Vec3{0.1, 0.1, 0.15}, Vec3{0, 0, kZ0});
    Leg legs[2];
    physics::GroundPlane ground;
    Real A_hip = 0.45, A_knee = 0.8;
    Real foot_r = 0.05;
    Real grf[2] = {0, 0};   // fuerza normal por pie (GRF) del último paso
    bool pin_z = true;      // rig de soporte: torso a altura fija. false = DOF vertical real
    Real z_k = 60, z_c = 10; // compliancia vertical de apoyo (solo con pin_z=false):
                             // resorte-amortiguador hacia kZ0 que absorbe el rebote
                             // del paso (tejido blando/rodilla de apoyo), evita fase
                             // de vuelo y mantiene marcha, no carrera

    Biped() {
        torso.inv_inertia_body = math::Mat3::diagonal(0, 0, 0);  // rig: sin cabeceo
        ground.mu = 2.0; ground.friction = 400;
        for (int s = 0; s < 2; ++s) {
            Leg& L = legs[s];
            L.thigh = RigidBody::make_box(0.6, Vec3{0.04, 0.04, 0.2}, Vec3{0, 0, kHipH - 0.2});
            L.shank = RigidBody::make_box(0.5, Vec3{0.04, 0.04, 0.2}, Vec3{0, 0, kHipH - 0.6});
            L.hip_j = {Vec3{0, 0, 0.2}, Vec3{0, 0, -(kZ0 - kHipH)}, 0.2};
            L.knee_j = {Vec3{0, 0, 0.2}, Vec3{0, 0, -0.2}, 0.2};
            L.hip_lim.axis = {0,1,0};  L.hip_lim.ref = {0,0,-1};  L.hip_lim.local_dir = {0,0,-1};
            L.hip_lim.lo = -0.7; L.hip_lim.hi = 0.7;
            L.knee_lim.axis = {0,1,0}; L.knee_lim.ref = {0,0,-1}; L.knee_lim.local_dir = {0,0,-1};
            L.knee_lim.lo = -1.5; L.knee_lim.hi = 0.7;
            addM(L, &torso, &L.thigh, {0.2, 0, -0.05}, {0, 0, 0.15});
            addM(L, &torso, &L.thigh, {-0.2, 0, -0.05}, {0, 0, 0.15});
            addM(L, &L.thigh, &L.shank, {0.15, 0, -0.15}, {0, 0, 0.15});
            addM(L, &L.thigh, &L.shank, {-0.15, 0, -0.15}, {0, 0, 0.15});
            L.m[0].act.l_opt = L.m[1].act.l_opt = 0.25;
            L.m[2].act.l_opt = L.m[3].act.l_opt = 0.18;
            for (int i = 0; i < 4; ++i) L.m[i].act.f_max = (i < 2) ? 500 : 350;
        }
    }
    void addM(Leg& L, RigidBody* a, RigidBody* b, Vec3 o, Vec3 i) {
        Act mm; mm.a = a; mm.b = b; mm.at.origin_local = o; mm.at.insertion_local = i;
        L.m.push_back(mm);
    }
    static Real c01(Real x) { return x > 1 ? 1 : (x < 0 ? 0 : x); }

    Real hip_angle(Leg& L) {
        return physics::signed_angle({0,1,0}, {0,0,-1}, L.thigh.orient.rotate({0,0,-1}));
    }
    Real knee_angle(Leg& L) {
        return physics::signed_angle({0,1,0}, L.thigh.orient.rotate({0,0,-1}),
                                     L.shank.orient.rotate({0,0,-1}));
    }
    Vec3 foot(Leg& L) { return L.shank.pos + L.shank.orient.rotate({0, 0, -0.2}); }

    void pd(Leg& L, int fa, int fb, Real target, Real angle, Real rate, Real kp, Real kd) {
        Real u = kp * (target - angle) - kd * rate;
        L.m[fa].act.activation = c01((u > 0 ? u : 0) + 0.03);
        L.m[fb].act.activation = c01((u < 0 ? -u : 0) + 0.03);
    }

    void step(Real dt, control::CoupledOscillators& cpg, bool walking) {
        for (int s = 0; s < 2; ++s) {
            Leg& L = legs[s];
            Real phi = cpg.phase[s];
            Real hip_t = walking ? -A_hip * std::cos(phi) : 0.0;
            Real knee_t = walking ? -A_knee * std::max(0.0, -std::sin(phi)) : 0.0;
            Real ha = hip_angle(L), ka = knee_angle(L);
            Real hr = L.thigh.omega.y, kr = L.shank.omega.y - L.thigh.omega.y;
            pd(L, 1, 0, hip_t, ha, hr, 18.0, 2.5);
            pd(L, 3, 2, knee_t, ka, kr, 18.0, 2.0);
            physics::apply_gravity(L.thigh, kGravity);
            physics::apply_gravity(L.shank, kGravity);
            for (auto& mm : L.m) actuators::apply_actuator(mm.act, mm.at, *mm.a, *mm.b);
            grf[s] = physics::resolve_ground_point(L.shank, foot(L), foot_r, ground);
        }
        torso.integrate_velocity(dt);
        for (int s = 0; s < 2; ++s) { legs[s].thigh.integrate_velocity(dt); legs[s].shank.integrate_velocity(dt); }
        for (int it = 0; it < 20; ++it) {
            for (int s = 0; s < 2; ++s) {
                physics::solve_ball(legs[s].thigh, torso, legs[s].hip_j, dt);
                physics::solve_ball(legs[s].shank, legs[s].thigh, legs[s].knee_j, dt);
                physics::solve_angle_limit(legs[s].thigh, legs[s].hip_lim, dt);
                physics::solve_angle_limit(legs[s].shank, legs[s].knee_lim, dt);
            }
        }
        torso.pos.y = 0; torso.orient = math::Quat::identity();
        torso.vel.y = 0; torso.omega = math::Zero3;
        if (pin_z) { torso.pos.z = kZ0; torso.vel.z = 0; }
        else torso.vel.z += ((kZ0 - torso.pos.z) * z_k - torso.vel.z * z_c) * dt;
        for (int s = 0; s < 2; ++s) {
            legs[s].thigh.vel *= 0.999; legs[s].thigh.omega *= 0.999;
            legs[s].shank.vel *= 0.999; legs[s].shank.omega *= 0.999;
        }
        torso.integrate_position(dt);
        for (int s = 0; s < 2; ++s) { legs[s].thigh.integrate_position(dt); legs[s].shank.integrate_position(dt); }
    }
};

// Registro temporal de una simulación de marcha (para el benchmark).
struct GaitTrace {
    Real dt = 0;
    std::vector<Real> t, torso_x, grf0, grf1, hip0;
};

// Simula 'secs' segundos. Devuelve desplazamiento neto del torso; llena 'trace' si != null.
inline Real simulate(bool walking, Real secs = 8.0, GaitTrace* trace = nullptr) {
    Biped b;
    agent::WalkIntention intent; intent.walk = walking; intent.effort = 1.0;
    control::CoupledOscillators cpg(2);
    cpg.omega = 2.0 * math::Pi * (walking ? intent.cadence_hz() : 1.0);
    cpg.offset[1] = math::Pi;
    cpg.phase[0] = 0.0; cpg.phase[1] = math::Pi;
    Real dt = 1.0 / 1000.0;
    Real x0 = b.torso.pos.x;
    int n = int(secs / dt);
    if (trace) trace->dt = dt;
    for (int i = 0; i < n; ++i) {
        if (walking) cpg.step(dt);
        b.step(dt, cpg, walking);
        if (trace) {
            trace->t.push_back(i * dt);
            trace->torso_x.push_back(b.torso.pos.x);
            trace->grf0.push_back(b.grf[0]);
            trace->grf1.push_back(b.grf[1]);
            trace->hip0.push_back(b.hip_angle(b.legs[0]));
        }
    }
    return b.torso.pos.x - x0;
}

}  // namespace soma::scenario
