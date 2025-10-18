#include <gtest/gtest.h>
#include <set>
#include "delegate/RoundRobinMatchGenerator.hpp"

using scheduling::RoundRobinMatchGenerator;
using scheduling::MatchPair;
using domain::Team;

TEST(RRGenerator, SixteenTeams_Produces120Pairs_NoSelf_NoDup) {
    std::vector<Team> teams;
    teams.reserve(16);
    for (int i = 0; i < 16; ++i) {
        teams.push_back(Team{std::string("T") + std::to_string(i+1),
                             std::string("Team ") + std::to_string(i+1)});
    }

    auto matches = RoundRobinMatchGenerator::Generate(teams);

    // C(16,2) = 120
    EXPECT_EQ(matches.size(), 120u);

    // sin self matches
    for (const auto& m : matches) {
        EXPECT_NE(m.homeId, m.awayId);
    }

    // no duplicados (A-B == B-A como par único)
    auto key = [](const MatchPair& m) {
        return (m.homeId < m.awayId) ? (m.homeId + "-" + m.awayId) : (m.awayId + "-" + m.homeId);
    };
    std::set<std::string> seen;
    for (const auto& m : matches) {
        seen.insert(key(m));
    }
    EXPECT_EQ(seen.size(), matches.size());
}

TEST(RRGenerator, OddNumber_HandledByBye) {
    std::vector<Team> teams = {
        {"A","A"}, {"B","B"}, {"C","C"} // 3 equipos => C(3,2)=3 partidos
    };
    auto matches = RoundRobinMatchGenerator::Generate(teams);
    EXPECT_EQ(matches.size(), 3u);
}
