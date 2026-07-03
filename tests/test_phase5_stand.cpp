// SOMA — Test FASE 5 (equilibrio SIN rig ni pin): de pie sobre un pie físico.
//
// A diferencia del péndulo invertido anclado, aquí NO hay pin ni rig: el cuerpo se
// sostiene sobre un PIE real (planta con talón y punta) apoyado en el suelo por
// contacto y fricción. El equilibrio se mantiene con la estrategia de tobillo
// (vestíbulo → activación de músculos del tobillo). El pie puede resbalar o volcar
// si el control falla. Se verifica que el cuerpo se mantiene erguido y recupera de
// una inclinación, sin caer — equilibrio 100 % emergente y sin idealizaciones.
#include "anatomy/muscle/hill_muscle.hpp"
#include "brain/balance/postural_control.hpp"
#include "physics/collision/ground.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"
#include "sensory/vestibular/vestibular.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using anatomy::HillMuscle;
using anatomy::MuscleAttachment;

constexpr Vec3 kGravity{0, 0, -9.80665};

struct Stander {
    // Pie: placa plana sobre el suelo (talón−punta a lo largo de X).
    RigidBody foot = RigidBody::make_box(0.6, Vec3{0.12, 0.06, 0.02}, Vec3{0, 0, 0.02});
    // Cuerpo: barra vertical, CM a media altura, articulada al pie por el tobillo.
    RigidBody body = RigidBody::make_box(4.0, Vec3{0.05, 0.05, 0.5}, Vec3{0, 0, 0.54});
    physics::BallJoint ankle{Vec3{0, 0, -0.5}, Vec3{0, 0, 0.02}, 0.2};  // base del cuerpo ↔ tobillo
    physics::GroundPlane ground;
    HillMuscle posterior, anterior;
    MuscleAttachment post_at, ant_at;
    sensory::Vestibular vest;
    brain::BalanceController ctrl;

    Stander(Real tilt0) {
        ground.mu = 1.5; ground.friction = 500;
        for (HillMuscle* m : {&posterior, &anterior}) { m->f_max = 600; m->l_opt = 0.3; }
        // Tobillo en (0,0,0.04). Músculos del pie (±X) a un punto del cuerpo por encima.
        post_at.origin_local = Vec3{-0.1, 0, -0.02}; post_at.insertion_local = Vec3{0, 0, -0.25};
        ant_at.origin_local = Vec3{0.1, 0, -0.02};   ant_at.insertion_local = Vec3{0, 0, -0.25};
        // Inclina el cuerpo tilt0 alrededor del tobillo.
        math::Quat q = math::Quat::from_axis_angle(Vec3{0, 1, 0}, tilt0);
        body.orient = q;
        body.pos = Vec3{0, 0, 0.04} + q.rotate(Vec3{0, 0, 0.5});
    }

    // Contactos del pie: talón (−X) y punta (+X), ambos en la planta.
    void foot_contacts() {
        Vec3 heel = foot.pos + foot.orient.rotate(Vec3{-0.12, 0, -0.02});
        Vec3 toe = foot.pos + foot.orient.rotate(Vec3{0.12, 0, -0.02});
        physics::resolve_ground_point(foot, heel, 0.0, ground);
        physics::resolve_ground_point(foot, toe, 0.0, ground);
    }

    void step(Real dt) {
        Real tilt = vest.tilt(body), rate = vest.pitch_rate(body);
        auto out = ctrl.command(tilt, rate);
        posterior.activation = out.posterior;
        anterior.activation = out.anterior;
        physics::apply_gravity(body, kGravity);
        physics::apply_gravity(foot, kGravity);
        anatomy::apply_muscle(posterior, post_at, foot, body);
        anatomy::apply_muscle(anterior, ant_at, foot, body);
        foot_contacts();
        body.integrate_velocity(dt);
        foot.integrate_velocity(dt);
        for (int it = 0; it < 20; ++it) physics::solve_ball(body, foot, ankle, dt);
        body.vel *= 0.999; body.omega *= 0.999;
        foot.vel *= 0.999; foot.omega *= 0.999;
        body.integrate_position(dt);
        foot.integrate_position(dt);
    }
};

static void run(Real tilt0, Real kp, Real kd, Real& max_tilt, Real& final_tilt) {
    Stander s(tilt0);
    s.ctrl.kp = kp; s.ctrl.kd = kd;
    Real dt = 1.0 / 1000.0;
    max_tilt = 0;
    for (int i = 0; i < 6000; ++i) {   // 6 s
        s.step(dt);
        if (i > 200) max_tilt = std::max(max_tilt, std::fabs(s.vest.tilt(s.body)));
    }
    final_tilt = std::fabs(s.vest.tilt(s.body));
}

int main() {
    // Con control: parte inclinado 0.08 rad y se mantiene erguido sin caer.
    Real mx, fn;
    run(0.08, 9.0, 2.0, mx, fn);
    std::fprintf(stderr, "de pie (control): max_tilt=%.3f final_tilt=%.3f\n", mx, fn);
    assert(mx < 0.30);     // nunca se desploma
    assert(fn < 0.06);     // se endereza y se mantiene

    // Sin control: se cae (equilibrio activo, no truco).
    Real mx0, fn0;
    run(0.08, 0.0, 0.0, mx0, fn0);
    std::fprintf(stderr, "de pie (sin control): max_tilt=%.3f final_tilt=%.3f\n", mx0, fn0);
    assert(fn0 > 0.4);     // cae

    return 0;
}
