// SOMA — Test FASE 2: el MÚSCULO mueve el HUESO.
//
// Primer momento biológico. Un hueso cuelga de un pivote. Un músculo Hill va de
// un punto fijo por encima del pivote a una inserción en el hueso. Al ACTIVARLO,
// el músculo se contrae, tira de la inserción y FLEXIONA el hueso hacia arriba.
// Cero animación: activación -> fuerza Hill -> par -> el hueso rota por física.
//
// Además valida las propiedades del modelo Hill (curvas F-L, F-V, pasiva).
#include "anatomy/muscle/hill_muscle.hpp"
#include "physics/constraints/ball_joint.hpp"
#include "physics/constraints/joint_limit.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using anatomy::HillMuscle;
using anatomy::MuscleAttachment;

constexpr Vec3 kGravity{0, 0, -9.80665};

// --- Propiedades del modelo Hill ---
static void test_hill_properties() {
    HillMuscle m;
    m.f_max = 800; m.l_opt = 0.6;

    // Fuerza-longitud: pico en la longitud óptima.
    Real f_opt = m.force_length(0.6);
    assert(std::fabs(f_opt - 1.0) < 1e-6);
    assert(m.force_length(0.6 * 0.7) < f_opt);   // demasiado corto
    assert(m.force_length(0.6 * 1.3) < f_opt);   // demasiado largo

    // Fuerza-velocidad: 1 en isométrico, menos al acortar, más al alargar.
    assert(std::fabs(m.force_velocity(0.0) - 1.0) < 1e-6);
    assert(m.force_velocity(-1.0) < 1.0);        // acortando => menos fuerza
    Real f_ecc = m.force_velocity(+1.0);
    assert(f_ecc > 1.0 && f_ecc <= m.f_ecc + 1e-9);  // alargando => más, con meseta

    // Pasiva: nula en/por debajo de l_opt, positiva al estirar.
    assert(m.force_passive(0.6) == 0.0);
    assert(m.force_passive(0.6 * 1.5) > 0.0);

    // Tensión: sin activación en l_opt => 0; activación plena en l_opt => ~Fmax.
    m.length = 0.6; m.velocity = 0.0;
    m.activation = 0.0; assert(m.tension() == 0.0);
    m.activation = 1.0; assert(std::fabs(m.tension() - 800.0) < 1e-6);
}

// Simula el hueso con una activación dada. Devuelve el ángulo final (rad).
// theta ~0 = horizontal, ~+90° = colgando hacia abajo.
static Real run_with_activation(Real activation) {
    RigidBody anchor = RigidBody::make_static(Vec3{0, 0, 0});
    RigidBody bone = RigidBody::make_box(1.0, Vec3{0.5, 0.05, 0.05}, Vec3{0.5, 0, 0});

    physics::BallJoint pin{Vec3{-0.5, 0, 0}, Vec3{0, 0, 0}, 0.2};

    HillMuscle biceps;
    biceps.f_max = 800; biceps.l_opt = 0.6; biceps.v_max = 10;
    biceps.activation = activation;
    MuscleAttachment at;
    at.origin_local = Vec3{0, 0, 0.5};      // punto fijo por encima del pivote
    at.insertion_local = Vec3{-0.2, 0, 0};  // inserción en el hueso

    Real dt = 1.0 / 1000.0;
    for (int step = 0; step < 4000; ++step) {      // 4 s
        physics::apply_gravity(bone, kGravity);
        anatomy::apply_muscle(biceps, at, anchor, bone);
        bone.integrate_velocity(dt);
        for (int it = 0; it < 12; ++it) physics::solve_ball(bone, anchor, pin, dt);
        // Amortiguación pasiva de tejido (los miembros reales son ~sobreamortiguados).
        bone.vel *= 0.999; bone.omega *= 0.999;
        bone.integrate_position(dt);
    }
    Vec3 r = bone.orient.rotate(Vec3{1, 0, 0});
    return physics::signed_angle(Vec3{0, 1, 0}, Vec3{1, 0, 0}, r);
}

int main() {
    test_hill_properties();

    // Respuesta graduada: a más activación, más flexión (menor ángulo).
    // Esto prueba que es el MÚSCULO quien mueve el hueso, no un truco.
    Real a[] = {0.0, 0.25, 0.5, 1.0};
    Real theta[4];
    for (int i = 0; i < 4; ++i) {
        theta[i] = run_with_activation(a[i]);
        std::fprintf(stderr, "activacion=%.2f -> theta=%.3f rad (%.1f°)\n",
                     a[i], theta[i], theta[i] * 180.0 / math::Pi);
    }

    // Monótono: cada incremento de activación flexiona más el hueso.
    for (int i = 1; i < 4; ++i) assert(theta[i] < theta[i - 1]);
    // Efecto grande: activar a tope levanta el hueso mucho más que relajado.
    assert(theta[0] - theta[3] > 0.8);

    return 0;
}
