#pragma once
#include <string_view>

class IMatchScheduler {
public:
    virtual ~IMatchScheduler() = default;

    // Programa partidos de Round Robin cuando el grupo está completo.
    virtual void ScheduleRoundRobin(std::string_view tournamentId, std::string_view groupId) = 0;
};
