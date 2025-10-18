#pragma once
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include "domain/Team.hpp"
#include "domain/Match.hpp"

class RoundRobinGenerator {
public:
    // Declara lo que define tu .cpp
    static std::vector<std::pair<std::string, std::string>>
    GenerateSingleRound(const std::vector<std::string>& teamIdsInput);

    // Generador de todo el fixture RR (usa "circle method"; maneja BYE si es impar)
    static std::vector<domain::Match>
    Generate(const std::vector<domain::Team>& teams,
             const std::string& tournamentId,
             const std::string& groupId)
    {
        using namespace domain;
        std::vector<Match> out;
        const int originalN = static_cast<int>(teams.size());
        if (originalN < 2) return out;

        std::vector<int> idx;
        idx.reserve((originalN % 2 == 0) ? originalN : (originalN + 1));
        for (int i = 0; i < originalN; ++i) idx.push_back(i);
        bool odd = (originalN % 2) == 1;
        if (odd) idx.push_back(-1);

        int n = static_cast<int>(idx.size());
        int rounds = n - 1;
        out.reserve((originalN * (originalN - 1)) / 2);

        for (int r = 0; r < rounds; ++r) {
            for (int i = 0; i < n / 2; ++i) {
                int a = idx[i];
                int b = idx[n - 1 - i];
                if (a == -1 || b == -1) continue;

                Match m;
                m.TournamentId = tournamentId;
                m.GroupId      = groupId;
                if ((r % 2) == 0) {
                    m.HomeTeamId = teams[a].Id;
                    m.AwayTeamId = teams[b].Id;
                } else {
                    m.HomeTeamId = teams[b].Id;
                    m.AwayTeamId = teams[a].Id;
                }
                m.Phase = MatchPhase::RoundRobin;
                m.Round = r + 1;
                out.push_back(std::move(m));
            }
            std::rotate(idx.begin() + 1, idx.end() - 1, idx.end());
        }
        return out;
    }
};
