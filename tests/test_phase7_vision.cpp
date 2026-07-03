// SOMA — Test FASE 7 (visión): el agente VE, no lee coordenadas.
//
// El cámara proyecta objetos del mundo sobre la sensor. El agente recibe SOLO la
// imagen retiniana (u,v) y la orientación de su cámara, y reconstruye la dirección
// hacia el objeto. Se verifica que la dirección percibida coincide con la real
// SIN que el agente lea nunca la posición del objeto (causalidad). Además: objetos
// detrás o fuera del campo no se ven; girar el cámara cambia lo que se percibe.
#include "agent/perception/visual_processing.hpp"
#include "sensors/vision/camera.hpp"

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
    sensors::Camera eye;                 // en el origen, mirando +X
    agent::VisionUnit cortex;

    // --- Un objeto visible: el agente deduce su dirección solo desde la sensor ---
    Vec3 object{5.0, 1.0, 0.5};       // posición REAL (la conoce el cámara, NO el agente)
    auto r = eye.project(object);
    assert(r.visible);

    // El agente solo recibe r.u, r.v (imagen), la focal y la orientación del cámara.
    Vec3 perceived = cortex.direction_from_sensor(r.u, r.v, eye.focal, eye.orient);
    Vec3 truth = object - eye.pos;    // dirección real (solo para comprobar el test)
    std::fprintf(stderr, "objeto: sensor=(%.3f,%.3f)  percibida=(%.3f,%.3f,%.3f)  real=(%.3f,%.3f,%.3f)\n",
                 r.u, r.v, perceived.x, perceived.y, perceived.z,
                 math::normalize(truth).x, math::normalize(truth).y, math::normalize(truth).z);
    assert(same_dir(perceived, truth, 1e-9));   // percibe la dirección correcta

    // --- Mover el objeto → la percepción sigue (desde la nueva imagen) ---
    Vec3 obj2{4.0, -1.5, 1.0};
    auto r2 = eye.project(obj2);
    assert(r2.visible);
    Vec3 p2 = cortex.direction_from_sensor(r2.u, r2.v, eye.focal, eye.orient);
    assert(same_dir(p2, obj2 - eye.pos, 1e-9));

    // --- Objeto detrás del cámara: no llega luz a la sensor → no se ve ---
    assert(!eye.project(Vec3{-5, 0, 0}).visible);

    // --- Objeto fuera del campo de visión (muy lateral) → no se ve ---
    assert(!eye.project(Vec3{1, 5, 0}).visible);   // u = 5 > sensor_half

    // --- Girar el cámara hacia el objeto lo lleva a la fóvea (centro de la sensor) ---
    Vec3 side{5, 3, 0};
    Real yaw = std::atan2(3.0, 5.0);               // ángulo hacia el objeto
    eye.orient = math::Quat::from_axis_angle(Vec3{0, 0, 1}, yaw);  // gira en torno a +Z
    auto rc = eye.project(side);
    assert(rc.visible);
    assert(cortex.eccentricity(rc.u, rc.v) < 1e-6); // ahora está centrado en la fóvea
    // Y aun así el agente reconstruye la dirección correcta con su cámara girado.
    assert(same_dir(cortex.direction_from_sensor(rc.u, rc.v, eye.focal, eye.orient),
                    side - eye.pos, 1e-9));

    return 0;
}
