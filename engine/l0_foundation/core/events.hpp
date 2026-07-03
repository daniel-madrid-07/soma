// SOMA — L0 — bus de mensajes + blackboard.
//
// PIEZA CLAVE DE LA CAUSALIDAD. Los sistemas NO se llaman entre sí ni leen el
// estado interno de otro. Publican señales tipadas en el bus; otros las consumen.
// La señal es el ÚNICO acoplamiento. Esto hace la simulación "real", no truco.
//
// Modelo determinista: entrega en el mismo tick, en orden de dependencia
// (el scheduler ordena productores antes que consumidores). Al final del tick
// el bus se limpia. Retardos reales (conducción nerviosa) se modelan aparte,
// no aquí.
#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace soma::events {

// --- MessageBus: canales tipados por T ---
class MessageBus {
    struct ChannelBase {
        virtual ~ChannelBase() = default;
        virtual void clear() = 0;
    };
    template <class T>
    struct Channel : ChannelBase {
        std::vector<T> items;
        void clear() override { items.clear(); }
    };

public:
    template <class T>
    void publish(const T& msg) { channel<T>().items.push_back(msg); }

    template <class T>
    void publish(T&& msg) { channel<T>().items.push_back(std::move(msg)); }

    // Mensajes de tipo T publicados en este tick.
    template <class T>
    const std::vector<T>& messages() {
        return channel<T>().items;
    }

    template <class T>
    bool any() { return !channel<T>().items.empty(); }

    // Se llama al final de cada tick base.
    void clear_all() {
        for (auto& [k, ch] : channels_) ch->clear();
    }

private:
    template <class T>
    Channel<T>& channel() {
        auto key = std::type_index(typeid(T));
        auto it = channels_.find(key);
        if (it == channels_.end()) {
            auto ch = std::make_unique<Channel<T>>();
            auto* raw = ch.get();
            channels_.emplace(key, std::move(ch));
            return *raw;
        }
        return *static_cast<Channel<T>*>(it->second.get());
    }

    std::unordered_map<std::type_index, std::unique_ptr<ChannelBase>> channels_;
};

// --- Blackboard: estado compartido tipado, un slot por tipo ---
// Para valores que persisten entre ticks (set-points de homeostasis,
// estimación actual del centro de masa, etc.). Lectura sin acoplar sistemas.
class Blackboard {
    struct SlotBase { virtual ~SlotBase() = default; };
    template <class T>
    struct Slot : SlotBase { T value; explicit Slot(T v) : value(std::move(v)) {} };

public:
    template <class T>
    void set(T value) {
        auto key = std::type_index(typeid(T));
        slots_[key] = std::make_unique<Slot<T>>(std::move(value));
    }

    template <class T>
    const T* get() const {
        auto it = slots_.find(std::type_index(typeid(T)));
        if (it == slots_.end()) return nullptr;
        return &static_cast<Slot<T>*>(it->second.get())->value;
    }

    template <class T>
    T get_or(T fallback) const {
        const T* p = get<T>();
        return p ? *p : fallback;
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<SlotBase>> slots_;
};

}  // namespace soma::events
