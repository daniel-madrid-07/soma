// SOMA — Test FASE 1 (slice 1): dinámica de cuerpo rígido.
//
// Hito parcial del ROADMAP: un cuerpo cae por gravedad, choca con el suelo y
// REPOSA. Todo por física — cero animación. Además valida:
//   - integrador en caída libre vs analítico,
//   - conservación de momento angular sin par (giro constante),
//   - una fuerza fuera del centro genera rotación (así empuja un actuador a un hueso).
#include "core/assert.hpp"
#include "physics/collision/ground.hpp"
#include "physics/forces/gravity.hpp"
#include "physics/rigid/body.hpp"
#include "sim/scheduler.hpp"

#include <cassert>
#include <cmath>

using namespace soma;
using math::Real;
using math::Vec3;
using physics::RigidBody;
using physics::GroundPlane;

constexpr Vec3 kGravity{0, 0, -9.80665};

static bool near(Real a, Real b, Real tol) { return std::fabs(a - b) < tol; }

// --- 1) Caída libre: sin suelo, comparar con solución analítica ---
static void test_free_fall() {
    RigidBody b = RigidBody::make_sphere(1.0, 0.5, Vec3{0, 0, 100});
    Real dt = 1.0 / 1000.0;
    for (int i = 0; i < 1000; ++i) {         // 1 s
        physics::apply_gravity(b, kGravity);
        b.integrate(dt);
    }
    // v = -g·t. Semi-implícito da esto exacto para aceleración constante.
    assert(near(b.vel.z, -9.80665, 1e-9));
    // z ≈ 100 - 1/2 g t². El semi-implícito adelanta medio paso: tolerancia holgada.
    assert(near(b.pos.z, 100.0 - 0.5 * 9.80665, 0.02));
}

// --- 2) Sin par aplicado, la velocidad angular se conserva (inercia isótropa) ---
static void test_angular_conservation() {
    RigidBody b = RigidBody::make_sphere(2.0, 0.3, Vec3{0, 0, 0});
    b.omega = Vec3{0, 0, 5.0};               // gira a 5 rad/s
    Real dt = 1.0 / 1000.0;
    for (int i = 0; i < 2000; ++i) b.integrate(dt);  // 2 s
    assert(near(b.omega.z, 5.0, 1e-9));      // sin par => sin cambio
    assert(near(math::length(b.omega), 5.0, 1e-9));
    // La orientación avanzó: 5 rad/s · 2 s ≈ 10 rad. No es la identidad.
    assert(!near(b.orient.w, 1.0, 1e-3) || !near(b.orient.z, 0.0, 1e-3));
}

// --- 3) Fuerza fuera del centro => aparece rotación (actuador->hueso) ---
static void test_off_center_force() {
    RigidBody b = RigidBody::make_box(1.0, Vec3{0.5, 0.1, 0.1}, Vec3{0, 0, 0});
    // Fuerza +Y aplicada en el extremo +X: debe generar par alrededor de +Z.
    b.apply_force_at(Vec3{0, 10, 0}, Vec3{0.5, 0, 0});
    assert(b.torque.z > 0.0);                // par positivo en Z
    b.integrate(1.0 / 1000.0);
    assert(b.omega.z > 0.0);                 // empezó a rotar
    assert(b.vel.y > 0.0);                   // y también trasladó (componente lineal)
}

// --- 4) Caída + suelo => REPOSA sobre el suelo (por física, no scriptado) ---
static void test_fall_and_rest() {
    sim::World world(1000.0, 1);
    GroundPlane ground;                       // z=0, muelle-amortiguador
    ecs::Entity e = world.reg.create();
    world.reg.add(e, RigidBody::make_sphere(1.0, 0.5, Vec3{0, 0, 5.0}));

    sim::Scheduler sched;
    sched.add({"gravity", 1000.0, [](sim::Context& c) {
        c.reg().each<RigidBody>([](ecs::Entity, RigidBody& b) {
            physics::apply_gravity(b, kGravity);
        });
    }, {}});
    sched.add({"ground", 1000.0, [ground](sim::Context& c) {
        c.reg().each<RigidBody>([&](ecs::Entity, RigidBody& b) {
            physics::resolve_ground(b, ground);
        });
    }, {"gravity"}});
    sched.add({"integrate", 1000.0, [](sim::Context& c) {
        Real dt = c.dt;
        c.reg().each<RigidBody>([&](ecs::Entity, RigidBody& b) { b.integrate(dt); });
    }, {"ground"}});

    sched.run(world, 4000);                   // 4 s: cae y se estabiliza

    RigidBody* b = world.reg.get<RigidBody>(e);
    assert(b->pos.z > 0.0);                   // NO atravesó el suelo
    assert(near(b->pos.z, 0.5, 5e-3));        // reposa a z = radio (± unos mm de penetración)
    assert(near(b->vel.z, 0.0, 1e-2));        // en reposo
}

int main() {
    test_free_fall();
    test_angular_conservation();
    test_off_center_force();
    test_fall_and_rest();
    return 0;
}
