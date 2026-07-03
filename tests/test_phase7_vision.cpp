// SOMA — Test FASE 7 (visión): el cerebro VE, no lee coordenadas.
//
// El ojo proyecta objetos del mundo sobre la retina. El cerebro recibe SOLO la
// imagen retiniana (u,v) y la orientación de su ojo, y reconstruye la dirección
// hacia el objeto. Se verifica que la dirección percibida coincide con la real
// SIN que el cerebro lea nunca la posición del objeto (causalidad). Además: objetos
// detrás o fuera del campo no se ven; girar el ojo cambia lo que se percibe.
#include "brain/perception/visual_processing.hpp"
#include "sensory/vision/eye.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace soma;
using soma::math::Real;
using soma::math::Vec3;

static bool same_dir(Vec3 a, Vec3 b, Real tol = 1e-6) {
    return math::length(math::normalize(a) - math::normalize(b)) < tol;
}

int main() {
    sensory::Eye eye;                 // en el origen, mirando +X
    brain::VisualCortex cortex;

    // --- Un objeto visible: el cerebro deduce su dirección solo desde la retina ---
    Vec3 object{5.0, 1.0, 0.5};       // posición REAL (la conoce el ojo, NO el cerebro)
    auto r = eye.project(object);
    assert(r.visible);

    // El cerebro solo recibe r.u, r.v (imagen), la focal y la orientación del ojo.
    Vec3 perceived = cortex.direction_from_retina(r.u, r.v, eye.focal, eye.orient);
    Vec3 truth = object - eye.pos;    // dirección real (solo para comprobar el test)
    std::fprintf(stderr, "objeto: retina=(%.3f,%.3f)  percibida=(%.3f,%.3f,%.3f)  real=(%.3f,%.3f,%.3f)\n",
                 r.u, r.v, perceived.x, perceived.y, perceived.z,
                 math::normalize(truth).x, math::normalize(truth).y, math::normalize(truth).z);
    assert(same_dir(perceived, truth, 1e-9));   // percibe la dirección correcta

    // --- Mover el objeto → la percepción sigue (desde la nueva imagen) ---
    Vec3 obj2{4.0, -1.5, 1.0};
    auto r2 = eye.project(obj2);
    assert(r2.visible);
    Vec3 p2 = cortex.direction_from_retina(r2.u, r2.v, eye.focal, eye.orient);
    assert(same_dir(p2, obj2 - eye.pos, 1e-9));

    // --- Objeto detrás del ojo: no llega luz a la retina → no se ve ---
    assert(!eye.project(Vec3{-5, 0, 0}).visible);

    // --- Objeto fuera del campo de visión (muy lateral) → no se ve ---
    assert(!eye.project(Vec3{1, 5, 0}).visible);   // u = 5 > retina_half

    // --- Girar el ojo hacia el objeto lo lleva a la fóvea (centro de la retina) ---
    Vec3 side{5, 3, 0};
    Real yaw = std::atan2(3.0, 5.0);               // ángulo hacia el objeto
    eye.orient = math::Quat::from_axis_angle(Vec3{0, 0, 1}, yaw);  // gira en torno a +Z
    auto rc = eye.project(side);
    assert(rc.visible);
    assert(cortex.eccentricity(rc.u, rc.v) < 1e-6); // ahora está centrado en la fóvea
    // Y aun así el cerebro reconstruye la dirección correcta con su ojo girado.
    assert(same_dir(cortex.direction_from_retina(rc.u, rc.v, eye.focal, eye.orient),
                    side - eye.pos, 1e-9));

    return 0;
}
