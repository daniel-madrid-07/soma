// SOMA — L1 — ECS (Entity-Component-System).
//
// Entidad = estructura de partes (un fémur, un actuador, una motonodo).
// Componente = propiedad física/mecánica (masa, longitud de fibra, potencial).
// Sistema = simulador que opera sobre componentes (vive en el scheduler).
//
// Almacenamiento: sparse set por tipo de componente (iteración densa, rápida).
// Handles con validez: destruir una entidad elimina sus componentes.
#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace soma::ecs {

using Entity = std::uint32_t;
constexpr Entity kInvalid = 0xFFFFFFFFu;

// --- Almacén de un tipo de componente (sparse set) ---
class IStore {
public:
    virtual ~IStore() = default;
    virtual void remove(Entity e) = 0;
    virtual bool has(Entity e) const = 0;
};

template <class T>
class Store final : public IStore {
public:
    T& add(Entity e, T value) {
        ensure_sparse(e);
        if (sparse_[e] != kInvalid) {  // ya existe: sobrescribe
            dense_[sparse_[e]] = std::move(value);
            return dense_[sparse_[e]];
        }
        sparse_[e] = static_cast<std::uint32_t>(dense_.size());
        dense_.push_back(std::move(value));
        owners_.push_back(e);
        return dense_.back();
    }

    bool has(Entity e) const override {
        return e < sparse_.size() && sparse_[e] != kInvalid;
    }

    T* get(Entity e) {
        if (!has(e)) return nullptr;
        return &dense_[sparse_[e]];
    }

    void remove(Entity e) override {
        if (!has(e)) return;
        std::uint32_t idx = sparse_[e];
        std::uint32_t last = static_cast<std::uint32_t>(dense_.size() - 1);
        // swap-and-pop
        dense_[idx] = std::move(dense_[last]);
        owners_[idx] = owners_[last];
        sparse_[owners_[idx]] = idx;
        dense_.pop_back();
        owners_.pop_back();
        sparse_[e] = kInvalid;
    }

    // Iteración densa.
    std::size_t size() const { return dense_.size(); }
    Entity entity_at(std::size_t i) const { return owners_[i]; }
    T& at(std::size_t i) { return dense_[i]; }

    template <class Fn>
    void each(Fn&& fn) {
        for (std::size_t i = 0; i < dense_.size(); ++i) fn(owners_[i], dense_[i]);
    }

private:
    void ensure_sparse(Entity e) {
        if (e >= sparse_.size()) sparse_.resize(e + 1, kInvalid);
    }
    std::vector<std::uint32_t> sparse_;  // entity -> índice denso (o kInvalid)
    std::vector<T> dense_;
    std::vector<Entity> owners_;
};

// --- Registro de entidades y componentes ---
class Registry {
public:
    Entity create() {
        Entity e;
        if (!free_.empty()) {
            e = free_.back();
            free_.pop_back();
            alive_[e] = 1;
        } else {
            e = static_cast<Entity>(alive_.size());
            alive_.push_back(1);
        }
        return e;
    }

    void destroy(Entity e) {
        if (!valid(e)) return;
        for (auto& [k, s] : stores_) s->remove(e);
        alive_[e] = 0;
        free_.push_back(e);
    }

    bool valid(Entity e) const { return e < alive_.size() && alive_[e]; }
    std::size_t alive_count() const {
        std::size_t n = 0;
        for (auto a : alive_) n += a;
        return n;
    }

    template <class T>
    T& add(Entity e, T value) { return store<T>().add(e, std::move(value)); }

    template <class T>
    T* get(Entity e) { return store<T>().get(e); }

    template <class T>
    bool has(Entity e) { return store<T>().has(e); }

    template <class T>
    void remove(Entity e) { store<T>().remove(e); }

    // Itera todas las entidades con componente T: fn(Entity, T&).
    template <class T, class Fn>
    void each(Fn&& fn) { store<T>().each(std::forward<Fn>(fn)); }

    // Itera entidades que tienen A y B: fn(Entity, A&, B&).
    template <class A, class B, class Fn>
    void each(Fn&& fn) {
        auto& sa = store<A>();
        auto& sb = store<B>();
        for (std::size_t i = 0; i < sa.size(); ++i) {
            Entity e = sa.entity_at(i);
            if (B* b = sb.get(e)) fn(e, sa.at(i), *b);
        }
    }

    template <class T>
    Store<T>& store() {
        auto key = std::type_index(typeid(T));
        auto it = stores_.find(key);
        if (it == stores_.end()) {
            auto s = std::make_unique<Store<T>>();
            auto* raw = s.get();
            stores_.emplace(key, std::move(s));
            return *raw;
        }
        return *static_cast<Store<T>*>(it->second.get());
    }

private:
    std::vector<std::uint8_t> alive_;
    std::vector<Entity> free_;
    std::unordered_map<std::type_index, std::unique_ptr<IStore>> stores_;
};

}  // namespace soma::ecs
