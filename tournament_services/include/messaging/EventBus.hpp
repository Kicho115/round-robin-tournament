#ifndef COMMON_EVENTBUS_HPP
#define COMMON_EVENTBUS_HPP

#include "IEventBus.hpp"

// Implementación no-op (segura por defecto)
class NullEventBus : public IEventBus {
public:
    void Publish(std::string_view, const nlohmann::json&) override {}
};

#endif // COMMON_EVENTBUS_HPP
