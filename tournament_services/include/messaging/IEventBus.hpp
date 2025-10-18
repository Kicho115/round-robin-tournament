#pragma once
#include <string_view>
#include <nlohmann/json.hpp>

class IEventBus {
public:
    virtual ~IEventBus() = default;
    virtual void Publish(std::string_view topic, const nlohmann::json& payload) = 0;
};
