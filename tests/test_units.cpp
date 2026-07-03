// SOMA — test del sistema de unidades SI.
// Verifica: (1) el álgebra dimensional se resuelve en compilación,
//           (2) los valores numéricos son correctos,
//           (3) las operaciones ilegales NO compilan (ver test_units_fail.cpp).
#include "core/units.hpp"

#include <cassert>
#include <cmath>
#include <type_traits>

using namespace soma::units;
using namespace soma::units::literals;

// --- (1) Álgebra dimensional en compilación ---
// masa · aceleración => fuerza
static_assert(std::is_same_v<decltype(Kilograms{1} * MetersPerSec2{1}), Newtons>);
// fuerza · longitud => energía
static_assert(std::is_same_v<decltype(Newtons{1} * Meters{1}), Joules>);
// energía / tiempo => potencia
static_assert(std::is_same_v<decltype(Joules{1} / Seconds{1}), Watts>);
// velocidad = longitud / tiempo
static_assert(std::is_same_v<decltype(Meters{1} / Seconds{1}), MetersPerSec>);
// fuerza / área => presión
static_assert(std::is_same_v<decltype(Newtons{1} / (Meters{1} * Meters{1})), Pascals>);

static bool almost(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main() {
    // --- (2) Peso de un cuerpo de 70 kg: F = m·g ---
    Newtons weight = 70.0_kg * constants::g;
    assert(almost(weight.value, 70.0 * 9.80665));

    // Trabajo al levantar 70 kg un metro: W = F·d
    Joules work = weight * 1.0_m;
    assert(almost(work.value, 686.4655));

    // Potencia si toma 2 s: P = W/t
    Watts power = work / 2.0_s;
    assert(almost(power.value, 343.23275));

    // Potencial de membrana en reposo: -70 mV
    Volts vm = -70.0_mV;
    assert(almost(vm.value, -0.070));

    // Suma legal (misma dimensión)
    Meters d = 1.5_m + 0.5_m;
    assert(almost(d.value, 2.0));

    return 0;  // todos los asserts pasaron
}
