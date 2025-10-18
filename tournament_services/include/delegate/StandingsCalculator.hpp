#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "domain/Match.hpp"

namespace standings {

    struct Row {
        std::string TeamId;
        int Wins{0};
        int PointsFor{0};
        int PointsAgainst{0};

        bool operator<(const Row& o) const {
            if (Wins != o.Wins) return Wins > o.Wins;
            if (PointsFor != o.PointsFor) return PointsFor > o.PointsFor;
            return PointsAgainst < o.PointsAgainst;
        }
    };

    class StandingsCalculator {
    public:
        std::vector<Row> Compute(const std::vector<domain::Match>& matches) const;

        std::vector<std::string> TopN(const std::vector<Row>& table, int n) const;

        static int ClampScore(int v) {
            if (v < 0) return 0;
            if (v > 10) return 10;
            return v;
        }
    };

    inline void to_json(nlohmann::json& j, const Row& r) {
        j = nlohmann::json{
            {"teamId", r.TeamId},
            {"wins", r.Wins},
            {"pointsFor", r.PointsFor},
            {"pointsAgainst", r.PointsAgainst}
        };
    }

} // namespace standings
