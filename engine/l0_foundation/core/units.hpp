// SOMA — L0 Foundation — Unidades SI con chequeo dimensional en compilación.
//
// Invariante #3 del proyecto: unidades SI en todo, verificadas por el compilador.
// Sumar metros + segundos NO compila. Multiplicar N * m da J automáticamente.
//
// Base SI: kg (masa), m (longitud), s (tiempo), A (corriente),
//          K (temperatura), mol (cantidad).
// Se omiten candela (luminosidad) por ahora; la visión trabaja en radiometría propia.
#pragma once

#include <ratio>

namespace soma::units {

// Dimensión = exponentes enteros de las 6 magnitudes base.
template <int kg, int m, int s, int A, int K, int mol>
struct Dim {
    static constexpr int e_kg = kg, e_m = m, e_s = s, e_A = A, e_K = K, e_mol = mol;
};

template <class D1, class D2>
using DimMul = Dim<D1::e_kg + D2::e_kg, D1::e_m + D2::e_m, D1::e_s + D2::e_s,
                   D1::e_A + D2::e_A, D1::e_K + D2::e_K, D1::e_mol + D2::e_mol>;

template <class D1, class D2>
using DimDiv = Dim<D1::e_kg - D2::e_kg, D1::e_m - D2::e_m, D1::e_s - D2::e_s,
                   D1::e_A - D2::e_A, D1::e_K - D2::e_K, D1::e_mol - D2::e_mol>;

// Cantidad física: un valor double etiquetado con su dimensión.
// Coste cero en runtime (la dimensión vive solo en el tipo).
template <class D>
struct Quantity {
    double value = 0.0;

    constexpr Quantity() = default;
    constexpr explicit Quantity(double v) : value(v) {}

    // Suma/resta: solo entre misma dimensión. Distinta dimensión => no compila.
    constexpr Quantity operator+(Quantity o) const { return Quantity{value + o.value}; }
    constexpr Quantity operator-(Quantity o) const { return Quantity{value - o.value}; }
    constexpr Quantity operator-() const { return Quantity{-value}; }

    // Escalado por número adimensional.
    constexpr Quantity operator*(double s) const { return Quantity{value * s}; }
    constexpr Quantity operator/(double s) const { return Quantity{value / s}; }

    constexpr bool operator<(Quantity o) const { return value < o.value; }
    constexpr bool operator>(Quantity o) const { return value > o.value; }
    constexpr bool operator==(Quantity o) const { return value == o.value; }
};

// Multiplicación/división: combinan dimensiones automáticamente.
template <class D1, class D2>
constexpr Quantity<DimMul<D1, D2>> operator*(Quantity<D1> a, Quantity<D2> b) {
    return Quantity<DimMul<D1, D2>>{a.value * b.value};
}
template <class D1, class D2>
constexpr Quantity<DimDiv<D1, D2>> operator/(Quantity<D1> a, Quantity<D2> b) {
    return Quantity<DimDiv<D1, D2>>{a.value / b.value};
}
template <class D>
constexpr Quantity<D> operator*(double s, Quantity<D> q) {
    return Quantity<D>{s * q.value};
}

// --- Dimensiones nombradas ---
using Dimensionless = Dim<0, 0, 0, 0, 0, 0>;
using Mass          = Dim<1, 0, 0, 0, 0, 0>;
using Length        = Dim<0, 1, 0, 0, 0, 0>;
using Time          = Dim<0, 0, 1, 0, 0, 0>;
using Current       = Dim<0, 0, 0, 1, 0, 0>;
using Temperature   = Dim<0, 0, 0, 0, 1, 0>;
using Amount        = Dim<0, 0, 0, 0, 0, 1>;

using Velocity      = Dim<0, 1, -1, 0, 0, 0>;   // m/s
using Acceleration  = Dim<0, 1, -2, 0, 0, 0>;   // m/s^2
using Force         = Dim<1, 1, -2, 0, 0, 0>;   // N = kg·m/s^2
using Pressure      = Dim<1, -1, -2, 0, 0, 0>;  // Pa = N/m^2
using Energy        = Dim<1, 2, -2, 0, 0, 0>;   // J = N·m
using Power         = Dim<1, 2, -3, 0, 0, 0>;   // W = J/s
using Voltage       = Dim<1, 2, -3, -1, 0, 0>;  // V = W/A

// --- Alias legibles para declarar variables ---
using Scalar        = Quantity<Dimensionless>;
using Kilograms     = Quantity<Mass>;
using Meters        = Quantity<Length>;
using Seconds       = Quantity<Time>;
using Kelvin        = Quantity<Temperature>;
using Moles         = Quantity<Amount>;
using MetersPerSec  = Quantity<Velocity>;
using MetersPerSec2 = Quantity<Acceleration>;
using Newtons       = Quantity<Force>;
using Pascals       = Quantity<Pressure>;
using Joules        = Quantity<Energy>;
using Watts         = Quantity<Power>;
using Volts         = Quantity<Voltage>;

// --- Literales SI: escribir 70.0_kg, 9.81_m, -70.0_mV ---
inline namespace literals {
constexpr Kilograms     operator"" _kg(long double v)  { return Kilograms{(double)v}; }
constexpr Meters        operator"" _m(long double v)   { return Meters{(double)v}; }
constexpr Seconds       operator"" _s(long double v)   { return Seconds{(double)v}; }
constexpr Seconds       operator"" _ms(long double v)  { return Seconds{(double)v * 1e-3}; }
constexpr Kelvin        operator"" _K(long double v)   { return Kelvin{(double)v}; }
constexpr Newtons       operator"" _N(long double v)   { return Newtons{(double)v}; }
constexpr Pascals       operator"" _Pa(long double v)  { return Pascals{(double)v}; }
constexpr Joules        operator"" _J(long double v)   { return Joules{(double)v}; }
constexpr Watts         operator"" _W(long double v)   { return Watts{(double)v}; }
constexpr Volts         operator"" _V(long double v)   { return Volts{(double)v}; }
constexpr Volts         operator"" _mV(long double v)  { return Volts{(double)v * 1e-3}; }
}  // namespace literals

// Constantes físicas del entorno (Parameter DB las sobreescribe por escenario).
namespace constants {
constexpr MetersPerSec2 g{9.80665};  // gravedad estándar
}  // namespace constants

}  // namespace soma::units
