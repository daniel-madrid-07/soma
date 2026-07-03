// SOMA — L0 — aserciones e invariantes.
// SOMA_ASSERT: invariantes generales (activo en debug).
// SOMA_CAUSALITY: marca puntos donde un sistema NO debe leer estado prohibido.
//   En builds de verificación (SOMA_VERIFY) fuerza el chequeo; documenta intención.
#pragma once

#include <cstdio>
#include <cstdlib>

namespace soma::detail {
[[noreturn]] inline void fail(const char* kind, const char* expr, const char* file, int line) {
    std::fprintf(stderr, "%s FALLÓ: %s\n  en %s:%d\n", kind, expr, file, line);
    std::abort();
}
}  // namespace soma::detail

#ifndef NDEBUG
#define SOMA_ASSERT(cond)                                                    \
    do {                                                                     \
        if (!(cond)) ::soma::detail::fail("ASSERT", #cond, __FILE__, __LINE__); \
    } while (0)
#else
#define SOMA_ASSERT(cond) ((void)0)
#endif

// Invariante de causalidad: la condición debe ser cierta para no hacer trampa.
// Ej: SOMA_CAUSALITY(!brain_reads_world_position);
#define SOMA_CAUSALITY(cond)                                                 \
    do {                                                                     \
        if (!(cond)) ::soma::detail::fail("CAUSALITY", #cond, __FILE__, __LINE__); \
    } while (0)
