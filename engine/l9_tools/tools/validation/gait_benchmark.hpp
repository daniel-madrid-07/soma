// SOMA — L9 — banco de validación de la marcha.
//
// El tip #1 del proyecto: cada sistema debe poder probarse contra la realidad.
// Este módulo extrae métricas de marcha de las señales grabadas (posición del
// torso, GRF por pie) y permite compararlas con rangos de marcha humana.
#pragma once

#include "core/math/scalar.hpp"

#include <vector>

namespace soma::tools {

using math::Real;

struct GaitMetrics {
    Real speed = 0;          // m/s (velocidad media de avance)
    Real stride_hz = 0;      // zancadas por segundo de un pie
    Real duty_factor = 0;    // fracción del tiempo con el pie en apoyo
    Real peak_grf = 0;       // fuerza normal máxima (N)
    int  contacts = 0;       // nº de apoyos detectados
};

// Detecta apoyos y agrega métricas. La GRF del contacto de penalización vibra, así
// que primero se SUAVIZA (media móvil) y luego se detectan flancos con HISTÉRESIS
// (sube por 'hi', baja por 'lo') para contar zancadas reales, no el ruido.
inline GaitMetrics gait_metrics(const std::vector<Real>& t,
                                const std::vector<Real>& x,
                                const std::vector<Real>& grf_foot,
                                Real hi = 60.0, Real lo = 15.0, int window = 50) {
    GaitMetrics m;
    std::size_t n = t.size();
    if (n < 2) return m;

    Real duration = t.back() - t.front();
    if (duration > 1e-6) m.speed = (x.back() - x.front()) / duration;

    // Media móvil de la GRF.
    std::vector<Real> s(n, 0);
    Real acc = 0;
    for (std::size_t i = 0; i < n; ++i) {
        acc += grf_foot[i];
        if (i >= (std::size_t)window) acc -= grf_foot[i - window];
        s[i] = acc / std::min<std::size_t>(i + 1, window);
        if (grf_foot[i] > m.peak_grf) m.peak_grf = grf_foot[i];
    }

    // Estado de apoyo con histéresis + conteo de flancos de subida.
    std::size_t in_contact = 0;
    bool stance = s[0] > hi;
    for (std::size_t i = 0; i < n; ++i) {
        if (!stance && s[i] > hi) { stance = true; ++m.contacts; }
        else if (stance && s[i] < lo) { stance = false; }
        if (stance) ++in_contact;
    }
    m.duty_factor = static_cast<Real>(in_contact) / static_cast<Real>(n);
    if (duration > 1e-6) m.stride_hz = m.contacts / duration;
    return m;
}

}  // namespace soma::tools
