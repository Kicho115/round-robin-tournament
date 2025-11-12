#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <set>
#include "domain/Team.hpp"
#include "domain/Match.hpp"
#include "util/RoundRobinGenerator.hpp"

using namespace std;

TEST(RRGenerator32, ThirtyTwoTeams_Produces496_NoSelf_NoDup_31Rounds) {
    vector<domain::Team> teams;
    teams.reserve(32);
    for (int i = 1; i <= 32; ++i) {
        domain::Team t;
        t.Id = "T" + to_string(i);
        t.Name = "Team " + to_string(i);
        teams.push_back(t);
    }

    auto matches = RoundRobinGenerator::Generate(teams, "T-XX", "G-1");

    EXPECT_EQ(matches.size(), 32 * 31 / 2);

    set<pair<string,string>> seen;
    int maxRound = 0;
    for (const auto& m : matches) {
        EXPECT_NE(m.HomeTeamId, m.AwayTeamId);
        auto p = minmax(m.HomeTeamId, m.AwayTeamId);
        EXPECT_TRUE(seen.insert(p).second);
        EXPECT_EQ(m.Phase, domain::MatchPhase::RoundRobin);
        maxRound = max(maxRound, m.Round);
    }
    EXPECT_EQ(maxRound, 31);
}
