#include "util/StandingsCalculator.hpp"
#include <unordered_map>

using standings::Row;

std::vector<Row>
standings::StandingsCalculator::Compute(const std::vector<domain::Match>& matches) const {
    std::unordered_map<std::string, Row> map;

    for (const auto& m : matches) {
        if (!m.IsScored()) continue;

        int h = ClampScore(*m.HomeScore);
        int a = ClampScore(*m.AwayScore);

        auto& H = map[m.HomeTeamId];
        H.TeamId = m.HomeTeamId;
        H.PointsFor     += h;
        H.PointsAgainst += a;

        auto& A = map[m.AwayTeamId];
        A.TeamId = m.AwayTeamId;
        A.PointsFor     += a;
        A.PointsAgainst += h;

        if (h > a) H.Wins += 1;
        else if (a > h)   A.Wins += 1;
    }

    std::vector<Row> out;
    out.reserve(map.size());
    for (auto& kv : map) out.emplace_back(std::move(kv.second));
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string>
standings::StandingsCalculator::TopN(const std::vector<Row>& table, int n) const {
    std::vector<std::string> ids;
    ids.reserve(std::min<int>(n, (int)table.size()));
    for (int i = 0; i < (int)table.size() && i < n; ++i)
        ids.push_back(table[i].TeamId);
    return ids;
}

