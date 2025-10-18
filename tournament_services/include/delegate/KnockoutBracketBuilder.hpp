#pragma once
#include <string>
#include <vector>
#include "domain/Match.hpp"
#include "delegate/StandingsCalculator.hpp"

class KnockoutBracketBuilder {
public:
    static std::vector<domain::Match>
    BuildTop8(const std::vector<standings::Row>& table,
              const std::string& tournamentId);

    static std::vector<domain::Match>
    BuildTop8FromSeeds(const std::vector<std::string>& orderedTeamIds,
                       const std::string& tournamentId);
};
