// SOMA — L0 — logging estructurado y ligero.
#pragma once

#include <cstdio>
#include <string_view>

namespace soma::log {

enum class Level { Trace, Debug, Info, Warn, Error };

inline Level& threshold() {
    static Level t = Level::Info;
    return t;
}

inline const char* tag(Level l) {
    switch (l) {
        case Level::Trace: return "TRC";
        case Level::Debug: return "DBG";
        case Level::Info:  return "INF";
        case Level::Warn:  return "WRN";
        case Level::Error: return "ERR";
    }
    return "?";
}

template <class... Args>
void message(Level l, std::string_view fmt, Args... args) {
    if (l < threshold()) return;
    std::fprintf(stderr, "[%s] ", tag(l));
    std::fprintf(stderr, fmt.data(), args...);
    std::fprintf(stderr, "\n");
}

#define SOMA_LOG_INFO(...)  ::soma::log::message(::soma::log::Level::Info,  __VA_ARGS__)
#define SOMA_LOG_WARN(...)  ::soma::log::message(::soma::log::Level::Warn,  __VA_ARGS__)
#define SOMA_LOG_ERROR(...) ::soma::log::message(::soma::log::Level::Error, __VA_ARGS__)

}  // namespace soma::log
