#ifndef COMMON_EVENTBUS_HPP
#define COMMON_EVENTBUS_HPP

#include <string_view>
#include <memory>
#include <nlohmann/json.hpp>

// Interfaz de bus de eventos
class IEventBus {
public:
    virtual ~IEventBus() = default;
    virtual void Publish(std::string_view topic, const nlohmann::json& payload) = 0;
};

// Implementación no-op (segura por defecto)
class NullEventBus : public IEventBus {
public:
    void Publish(std::string_view, const nlohmann::json&) override {}
};

#endif // COMMON_EVENTBUS_HPP
