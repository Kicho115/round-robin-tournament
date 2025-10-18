#pragma once
#include <string>
#include <vector>
#include "domain/Team.hpp"

namespace scheduling {

    struct MatchPair {
        std::string homeId;
        std::string awayId;
    };

    class RoundRobinMatchGenerator {
    public:
        // Genera una sola vuelta de Round Robin.
        // Si N es impar, se agrega un "BYE" virtual y no se generan partidos contra BYE.
        static std::vector<MatchPair> Generate(const std::vector<domain::Team>& teams);
    };

} // namespace scheduling
