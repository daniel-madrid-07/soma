// SOMA — L2 — cuerpo rígido. Base de cada HUESO.
//
// Estado: posición del centro de masa, orientación, velocidades lineal y angular.
// Integración semi-implícita (simpléctica): estable para cuerpos rígidos a 1 kHz.
// inv_mass == 0 => cuerpo estático (masa infinita, no se mueve: p.ej. el suelo).
#pragma once

#include "core/math/mat.hpp"
#include "core/math/quat.hpp"
#include "core/math/transform.hpp"
#include "core/math/vec.hpp"

namespace soma::physics {

using math::Mat3;
using math::Quat;
using math::Real;
using math::Vec3;

struct RigidBody {
    Real mass = 1.0;
    Real inv_mass = 1.0;
    Mat3 inv_inertia_body = Mat3::identity();  // inversa del tensor de inercia (marco cuerpo)

    Vec3 pos = math::Zero3;      // centro de masa (mundo)
    Quat orient = Quat::identity();
    Vec3 vel = math::Zero3;      // velocidad lineal (mundo)
    Vec3 omega = math::Zero3;    // velocidad angular (mundo)

    Vec3 force = math::Zero3;    // acumulador de fuerza (mundo)
    Vec3 torque = math::Zero3;   // acumulador de par (mundo)

    Real radius = 0.5;           // collider esférico simple (contacto). Se refina en L2 collision.

    // --- Construcción ---
    static RigidBody make_sphere(Real mass, Real radius, Vec3 pos) {
        RigidBody b;
        b.mass = mass;
        b.inv_mass = mass > 0 ? 1.0 / mass : 0.0;
        Real I = 0.4 * mass * radius * radius;  // esfera sólida: 2/5 m r^2
        Real invI = I > 0 ? 1.0 / I : 0.0;
        b.inv_inertia_body = Mat3::diagonal(invI, invI, invI);
        b.pos = pos;
        b.radius = radius;
        return b;
    }

    static RigidBody make_box(Real mass, Vec3 half_extents, Vec3 pos) {
        RigidBody b;
        b.mass = mass;
        b.inv_mass = mass > 0 ? 1.0 / mass : 0.0;
        Real hx = half_extents.x, hy = half_extents.y, hz = half_extents.z;
        Real ix = (1.0 / 12.0) * mass * (4 * hy * hy + 4 * hz * hz);
        Real iy = (1.0 / 12.0) * mass * (4 * hx * hx + 4 * hz * hz);
        Real iz = (1.0 / 12.0) * mass * (4 * hx * hx + 4 * hy * hy);
        b.inv_inertia_body = Mat3::diagonal(ix > 0 ? 1 / ix : 0,
                                            iy > 0 ? 1 / iy : 0,
                                            iz > 0 ? 1 / iz : 0);
        b.pos = pos;
        b.radius = math::length(half_extents);
        return b;
    }

    static RigidBody make_static(Vec3 pos) {
        RigidBody b;
        b.mass = 0;
        b.inv_mass = 0;
        b.inv_inertia_body = Mat3::diagonal(0, 0, 0);
        b.pos = pos;
        return b;
    }

    bool is_static() const { return inv_mass == 0.0; }

    // Inversa del tensor de inercia en el mundo: R · Iinv_body · Rᵀ.
    Mat3 inv_inertia_world() const {
        Mat3 R = orient.to_mat3();
        return R * inv_inertia_body * R.transpose();
    }

    // --- Aplicación de fuerzas ---
    void apply_force_cm(Vec3 f) { force += f; }
    void apply_torque(Vec3 t) { torque += t; }

    // Fuerza en un punto del mundo: genera fuerza lineal + par = (r × f).
    // Así es como un ACTUADOR empuja un HUESO en su inserción.
    void apply_force_at(Vec3 f, Vec3 world_point) {
        force += f;
        torque += math::cross(world_point - pos, f);
    }

    Vec3 point_velocity(Vec3 world_point) const {
        return vel + math::cross(omega, world_point - pos);
    }

    math::Transform transform() const { return {orient, pos}; }

    // --- Integración semi-implícita, en dos fases ---
    // Fase 1: fuerzas -> velocidades. Se llama ANTES de resolver restricciones.
    void integrate_velocity(Real dt) {
        if (inv_mass == 0.0) { clear(); return; }
        vel += force * (inv_mass * dt);
        omega += inv_inertia_world() * (torque * dt);
        clear();
    }

    // Fase 2: velocidades -> posición/orientación. Se llama DESPUÉS de restricciones.
    void integrate_position(Real dt) {
        if (inv_mass == 0.0) return;
        pos += vel * dt;
        Quat wq{0, omega.x, omega.y, omega.z};
        Quat dq = wq * orient;
        orient = Quat{orient.w + 0.5 * dq.w * dt,
                      orient.x + 0.5 * dq.x * dt,
                      orient.y + 0.5 * dq.y * dt,
                      orient.z + 0.5 * dq.z * dt}
                     .normalized();
    }

    // Conveniencia: un paso completo sin restricciones.
    void integrate(Real dt) {
        integrate_velocity(dt);
        integrate_position(dt);
    }

    void clear() { force = math::Zero3; torque = math::Zero3; }
};

}  // namespace soma::physics
