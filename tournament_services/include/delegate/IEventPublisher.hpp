#pragma once
#include <string>

class IEventPublisher {
public:
    virtual ~IEventPublisher() = default;
    virtual void Publish(const std::string& topic, const std::string& payload) = 0;
};
