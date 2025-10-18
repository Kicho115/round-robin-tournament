#include "delegate/RoundRobinGenerator.hpp"
#include <algorithm>

std::vector<std::pair<std::string, std::string>>
RoundRobinGenerator::GenerateSingleRound(const std::vector<std::string>& teamIdsInput) {
    std::vector<std::pair<std::string, std::string>> games;
    if (teamIdsInput.size() < 2) return games;

    // Método del círculo. Si impar, agregamos BYE virtual.
    std::vector<std::string> teams = teamIdsInput;
    bool hadBye = false;
    if (teams.size() % 2 == 1) {
        teams.push_back("__BYE__");
        hadBye = true;
    }

    const int n = static_cast<int>(teams.size());
    const int rounds = n - 1;

    std::vector<std::string> left, right;
    left.reserve(n/2);
    right.reserve(n/2);
    for (int i = 0; i < n/2; ++i) {
        left.push_back(teams[i]);
    }
    for (int i = n/2; i < n; ++i) {
        right.push_back(teams[i]);
    }
    std::reverse(right.begin(), right.end());

    for (int r = 0; r < rounds; ++r) {
        for (int i = 0; i < n/2; ++i) {
            const auto& a = left[i];
            const auto& b = right[i];
            if (a != "__BYE__" && b != "__BYE__") {
                // una vuelta: a vs b
                games.emplace_back(a, b);
            }
        }
        // rotación
        std::string l0 = left.front();
        std::string r0 = right.front();

        right.erase(right.begin());
        right.push_back(left.back());
        left.pop_back();
        left.insert(left.begin() + 1, r0);
    }

    return games;
}
