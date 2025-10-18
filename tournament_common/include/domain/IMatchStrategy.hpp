// Created by tomas on 8/31/25.
#ifndef TOURNAMENTS_IMATCHSTRATEGY_HPP
#define TOURNAMENTS_IMATCHSTRATEGY_HPP

#include <string>
#include <vector>

namespace domain {
    struct Team;
    struct Match;
}

class IMatchStrategy {
public:
    virtual ~IMatchStrategy() = default;

    // Genera partidos de Round Robin (cap de rondas permitido para RR parcial).
    // Debe producir pares Home/Away sin auto-enfrentamientos ni duplicados.
    virtual std::vector<domain::Match>
    GenerateRoundRobin(const std::vector<domain::Team>& teams,
                       const std::string& tournamentId,
                       const std::string& groupId,
                       int roundsCap /* e.g., 15 para 32 equipos => 240 partidos */) const = 0;

    // Genera la llave de eliminación sencilla (KO) a partir de una lista
    // ordenada de teamIds (por standings). Debe formar cuartos, semis, final.
    virtual std::vector<domain::Match>
    GenerateKnockout(const std::vector<std::string>& orderedTeamIds,
                     const std::string& tournamentId) const = 0;
};

#endif // TOURNAMENTS_IMATCHSTRATEGY_HPP
