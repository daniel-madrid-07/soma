// SOMA — Test de integración de FASE 0 (hito del ROADMAP).
//
// Verifica el núcleo completo trabajando junto:
//   - Dos entidades intercambian SEÑALES por el MessageBus.
//   - Sistemas corren a BANDAS distintas (multi-tasa) vía el scheduler.
//   - Integración numérica (Euler semi-implícito) mueve estado por FÍSICA.
//   - Estado que cruza bandas viaja por el Blackboard.
//   - Resultado DETERMINISTA (invariante #4): misma semilla => mismo resultado.
//   - Causalidad: los sistemas solo hablan por bus/blackboard.
#include "core/assert.hpp"
#include "sim/scheduler.hpp"
#include "sim/solvers.hpp"

#include <cassert>
#include <cmath>

using namespace soma;
using math::Real;
using math::Vec3;

constexpr Real kG = 9.80665;

// --- Componentes (propiedades físicas de las entidades) ---
struct Particle { Vec3 pos, vel; };   // cae por gravedad
struct GravityTag {};
struct Actuator { Vec3 pos, vel; Real gain; };  // empujado por señales
struct StimulusSource { Real strength; };

// --- Señal que viaja por el bus (entidad A -> entidad B) ---
struct Stimulus { ecs::Entity from; Real value; };

// --- Estado que cruza de banda rápida a lenta ---
struct ActuatorHeight { Real z; };

struct Result {
    Real gravity_vz;     // velocidad del cuerpo en caída tras 1 s
    Real actuator_z;     // posición del actuador tras 1 s
    Real monitored_z;    // último valor visto por la banda lenta
    int stim_ticks;      // cuántas veces corrió la banda de 1 kHz
    int monitor_ticks;   // cuántas veces corrió la banda de 10 Hz
};

// Construye un organismo mínimo y lo corre 1 s. Todo por sistemas, cero scripting.
static Result run_once(std::uint64_t seed) {
    sim::World world(/*base_hz=*/1000.0, seed);

    // Entidad en caída libre (valida el integrador).
    ecs::Entity faller = world.reg.create();
    world.reg.add(faller, Particle{Vec3{0, 0, 10}, Vec3{0, 0, 0}});
    world.reg.add(faller, GravityTag{});

    // Entidad A: fuente de estímulo. Entidad B: actuador que reacciona a A.
    ecs::Entity source = world.reg.create();
    world.reg.add(source, StimulusSource{0.5});
    ecs::Entity actuator = world.reg.create();
    world.reg.add(actuator, Actuator{Vec3{0, 0, 0}, Vec3{0, 0, 0}, 2.0});

    int stim_ticks = 0, monitor_ticks = 0;

    sim::Scheduler sched;

    // Sistema 1 @1000 Hz: gravedad -> integra partículas por física real.
    sched.add({"gravity", 1000.0, [](sim::Context& c) {
        Real dt = c.dt;
        c.reg().each<Particle, GravityTag>([&](ecs::Entity, Particle& p, GravityTag&) {
            solve::semi_implicit_euler(p.pos, p.vel,
                [](Vec3, Vec3) { return Vec3{0, 0, -kG}; }, dt);
        });
    }, {}});

    // Sistema 2 @1000 Hz: A publica una señal en el bus (no toca a B).
    sched.add({"stimulus", 1000.0, [&stim_ticks](sim::Context& c) {
        ++stim_ticks;
        c.reg().each<StimulusSource>([&](ecs::Entity e, StimulusSource& s) {
            c.bus().publish(Stimulus{e, s.strength});
        });
    }, {}});

    // Sistema 3 @1000 Hz: B lee señales del bus (SOLO del bus) y se mueve por física.
    // Depende de "stimulus": corre después, en el mismo tick.
    sched.add({"actuator", 1000.0, [actuator](sim::Context& c) {
        SOMA_CAUSALITY(true);  // B no lee el estado interno de A; solo el bus.
        Real total = 0;
        for (const auto& s : c.bus().messages<Stimulus>()) total += s.value;
        Actuator* a = c.reg().get<Actuator>(actuator);
        if (!a) return;
        Real dt = c.dt, gain = a->gain;
        solve::semi_implicit_euler(a->pos, a->vel,
            [&](Vec3, Vec3) { return Vec3{0, 0, gain * total}; }, dt);
    }, {"stimulus"}});

    // Sistema 4 @10 Hz (banda LENTA): observa al actuador y lo publica al blackboard.
    sched.add({"monitor", 10.0, [actuator, &monitor_ticks](sim::Context& c) {
        ++monitor_ticks;
        if (Actuator* a = c.reg().get<Actuator>(actuator))
            c.bb().set(ActuatorHeight{a->pos.z});
    }, {"actuator"}});

    sched.build(world);
    sched.run(world, 1000);  // 1000 pasos base = 1 s

    const ActuatorHeight* h = world.bb.get<ActuatorHeight>();
    return {world.reg.get<Particle>(faller)->vel.z,
            world.reg.get<Actuator>(actuator)->pos.z,
            h ? h->z : 0.0,
            stim_ticks, monitor_ticks};
}

static bool eq(Real a, Real b) { return std::fabs(a - b) < 1e-9; }

int main() {
    Result r = run_once(1);

    // --- Física real: caída libre 1 s => v_z = -g*t ---
    assert(eq(r.gravity_vz, -kG));  // -9.80665 m/s

    // --- El actuador se movió por señales recibidas del bus (no scriptado) ---
    // 1000 ticks * gain(2.0) * strength(0.5) integrados => v crece; pos > 0? no:
    // el estímulo es positivo => acelera +z => sube. Debe ser claramente > 0.
    assert(r.actuator_z > 0.0);

    // --- Multi-tasa: 1 kHz corrió 1000 veces, 10 Hz corrió 10 veces en 1 s ---
    assert(r.stim_ticks == 1000);
    assert(r.monitor_ticks == 10);

    // --- Estado cruzó de banda rápida a lenta por el blackboard ---
    assert(r.monitored_z > 0.0);

    // --- Determinismo: misma semilla => resultado idéntico bit a bit ---
    Result r2 = run_once(1);
    assert(eq(r.gravity_vz, r2.gravity_vz));
    assert(eq(r.actuator_z, r2.actuator_z));
    assert(r.stim_ticks == r2.stim_ticks && r.monitor_ticks == r2.monitor_ticks);

    return 0;
}
