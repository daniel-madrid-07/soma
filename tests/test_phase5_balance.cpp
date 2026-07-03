// SOMA — Test FASE 5: EQUILIBRIO. El cuerpo se mantiene erguido (péndulo invertido).
//
// El cuerpo es un péndulo invertido articulado en el tobillo: INESTABLE. La gravedad
// amplifica cualquier inclinación. Sólo un lazo de realimentación real (vestíbulo →
// control → activación → músculos Hill) puede mantenerlo de pie. Se verifica:
//   1. con control, se yergue desde una inclinación inicial y NO cae,
//   2. sin control, CAE (prueba de que el equilibrio es activo, no un truco),
//   3. tras un EMPUJÓN, se recupera solo.
// Cero animación: el equilibrio EMERGE de sentir y corregir.
#include "anatomy/muscle/hill_muscle.hpp"
#include "brain/balance/postural_control.hpp"
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

struct Stance {
    RigidBody foot = RigidBody::make_static(Vec3{0, 0, 0});
    RigidBody body = RigidBody::make_box(1.0, Vec3{0.05, 0.05, 0.5}, Vec3{0, 0, 0.5});
    physics::BallJoint ankle{Vec3{0, 0, -0.5}, Vec3{0, 0, 0}, 0.2};   // base al tobillo
    HillMuscle posterior, anterior;
    MuscleAttachment post_at, ant_at;
    sensory::Vestibular vest;
    brain::BalanceController ctrl;
    bool active = true;   // false => sin control ni músculos (péndulo invertido puro)

    Stance(Real kp, Real kd, Real tilt0) {
        for (HillMuscle* m : {&posterior, &anterior}) { m->f_max = 500; m->l_opt = 0.28; }
        post_at.origin_local = Vec3{-0.2, 0, 0}; post_at.insertion_local = Vec3{0, 0, -0.3};
        ant_at.origin_local = Vec3{0.2, 0, 0};   ant_at.insertion_local = Vec3{0, 0, -0.3};
        ctrl.kp = kp; ctrl.kd = kd;
        // Inclina el cuerpo tilt0 rad manteniendo la base en el tobillo.
        math::Quat q = math::Quat::from_axis_angle(Vec3{0, 1, 0}, tilt0);
        body.orient = q;
        body.pos = q.rotate(Vec3{0, 0, 0.5});
    }

    void step(Real dt) {
        if (active) {
            Real tilt = vest.tilt(body);
            Real rate = vest.pitch_rate(body);
            auto out = ctrl.command(tilt, rate);
            posterior.activation = out.posterior;
            anterior.activation = out.anterior;
            anatomy::apply_muscle(posterior, post_at, foot, body);
            anatomy::apply_muscle(anterior, ant_at, foot, body);
        }
        physics::apply_gravity(body, kGravity);
        body.integrate_velocity(dt);
        for (int it = 0; it < 15; ++it) physics::solve_ball(body, foot, ankle, dt);
        body.vel *= 0.9997; body.omega *= 0.9997;
        body.integrate_position(dt);
    }
};

// Corre y devuelve inclinación máxima y final (rad).
static void run(Real kp, Real kd, Real tilt0, Real& max_tilt, Real& final_tilt,
                Real push_at = -1, Real push = 0, bool active = true) {
    Stance s(kp, kd, tilt0);
    s.active = active;
    Real dt = 1.0 / 1000.0;
    max_tilt = 0;
    int push_step = (push_at >= 0) ? int(push_at / dt) : -1;
    for (int i = 0; i < 5000; ++i) {   // 5 s
        if (i == push_step) {
            // Empujón = desplazar la inclinación 'push' rad girando en torno al tobillo.
            math::Quat q = math::Quat::from_axis_angle(Vec3{0, 1, 0}, push);
            s.body.orient = q * s.body.orient;
            s.body.pos = q.rotate(s.body.pos);
            s.body.vel = math::Zero3; s.body.omega = math::Zero3;
        }
        s.step(dt);
        // Ignora el instante del salto para medir el pico real tras el empujón.
        if (i != push_step) max_tilt = std::max(max_tilt, std::fabs(s.vest.tilt(s.body)));
    }
    final_tilt = std::fabs(s.vest.tilt(s.body));
}

int main() {
    Real mx, fn;

    // 1) Con control: se yergue desde 0.12 rad y se mantiene.
    run(6.0, 1.2, 0.12, mx, fn);
    std::fprintf(stderr, "con control: max=%.3f final=%.3f\n", mx, fn);
    assert(mx < 0.25);      // nunca se desploma
    assert(fn < 0.03);      // se endereza

    // 2) Sin músculos activos: CAE (prueba de que el equilibrio es activo, no un truco).
    Real mx0, fn0;
    run(0.0, 0.0, 0.12, mx0, fn0, -1, 0, /*active=*/false);
    std::fprintf(stderr, "sin control: max=%.3f final=%.3f\n", mx0, fn0);
    assert(fn0 > 0.8);      // se ha ido al suelo

    // 3) Empujón a los 2 s partiendo erguido: se desvía 0.2 rad y se recupera.
    Real mxp, fnp;
    run(6.0, 1.2, 0.0, mxp, fnp, /*push_at=*/2.0, /*push=*/0.2);
    std::fprintf(stderr, "empujon: max=%.3f final=%.3f\n", mxp, fnp);
    assert(mxp > 0.1);      // el empujón lo desvió
    assert(fnp < 0.03);     // volvió a erguirse solo

    return 0;
}
