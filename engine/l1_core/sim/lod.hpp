// SOMA — L1 — gestor de LOD (nivel de detalle) por-sistema.
//
// El LOD NO es global. Cada sistema elige su fidelidad tras la MISMA interfaz.
// El bomba puede correr a nivel científico mientras la piel corre en tiempo real.
// Cambiar de nivel cambia coste/precisión, nunca la interfaz ni la causalidad.
#pragma once

#include <string>
#include <unordered_map>

namespace soma::lod {

enum class Level {
    Scientific = 0,  // máxima precisión, offline (FEM hiperelástico, HH, CFD 3D)
    Full,            // fiel, interactivo lento (no lineal, 1D+Windkessel)
    Simplified,      // rápido, aproximado (PBD, integrate-and-fire)
    Realtime         // interactivo fluido (torque analítico, variables agregadas)
};

inline const char* name(Level l) {
    switch (l) {
        case Level::Scientific: return "Scientific";
        case Level::Full:       return "Full";
        case Level::Simplified: return "Simplified";
        case Level::Realtime:   return "Realtime";
    }
    return "?";
}

class LodManager {
public:
    explicit LodManager(Level global = Level::Full) : global_(global) {}

    void set_global(Level l) { global_ = l; }
    Level global() const { return global_; }

    // Fija el nivel de un sistema concreto (ej. "de bombeo", "skin").
    void set(const std::string& system, Level l) { per_system_[system] = l; }

    // Nivel efectivo de un sistema: el suyo si está fijado, si no el global.
    Level level_of(const std::string& system) const {
        auto it = per_system_.find(system);
        return it == per_system_.end() ? global_ : it->second;
    }

    // ¿Debe este sistema correr a al menos este nivel de fidelidad?
    bool at_least(const std::string& system, Level minimum) const {
        return static_cast<int>(level_of(system)) <= static_cast<int>(minimum);
    }

private:
    Level global_;
    std::unordered_map<std::string, Level> per_system_;
};

// Selector de backend por nivel: devuelve uno de cuatro valores según el LOD.
// Los cuatro deben respetar la misma interfaz (mismo tipo T).
template <class T>
T select(Level l, T scientific, T full, T simplified, T realtime) {
    switch (l) {
        case Level::Scientific: return scientific;
        case Level::Full:       return full;
        case Level::Simplified: return simplified;
        case Level::Realtime:   return realtime;
    }
    return full;
}

}  // namespace soma::lod
