#include "delegate/RoundRobinMatchGenerator.hpp"
#include <algorithm>

using scheduling::MatchPair;
using scheduling::RoundRobinMatchGenerator;

std::vector<MatchPair> RoundRobinMatchGenerator::Generate(const std::vector<domain::Team>& inputTeams) {
    std::vector<MatchPair> matches;

    if (inputTeams.empty()) return matches;

    // Solo necesitamos IDs para hacer el fixture
    std::vector<std::string> ids;
    ids.reserve(inputTeams.size());
    for (const auto& t : inputTeams) ids.push_back(t.Id);

    if (ids.size() % 2 != 0) {
        ids.push_back("__BYE__"); // impar => insertar BYE
    }

    const int n = static_cast<int>(ids.size());
    const int rounds = n - 1;
    const int half = n / 2;

    // Método de "círculo" (Berger)
    std::vector<std::string> arr = ids;

    for (int round = 0; round < rounds; ++round) {
        for (int i = 0; i < half; ++i) {
            const std::string& a = arr[i];
            const std::string& b = arr[n - 1 - i];
            if (a == "__BYE__" || b == "__BYE__") continue;
            matches.push_back(MatchPair{a, b});
        }
        // rotar manteniendo arr[0] fijo
        std::rotate(arr.begin() + 1, arr.end() - 1, arr.end());
    }

    return matches;
}
