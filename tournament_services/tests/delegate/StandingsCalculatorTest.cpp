#include <gtest/gtest.h>
#include <vector>
#include "delegate/StandingsCalculator.hpp"
#include "domain/Match.hpp"

using namespace standings;
using namespace domain;

static Match scored(const std::string& h, const std::string& a, int hs, int as) {
    Match m;
    m.TournamentId = "T";
    m.GroupId = "G";
    m.HomeTeamId = h;
    m.AwayTeamId = a;
    m.Phase = MatchPhase::RoundRobin;
    m.Round = 1;
    m.SetScore(hs, as);
    return m;
}

TEST(Standings, ClampAndTieBreakers) {
    std::vector<Match> ms = {
        scored("T1","T2", 12, -3),
        scored("T1","T3",  5,  5),
        scored("T2","T3", 11, 15)
    };

    StandingsCalculator calc;
    auto table = calc.Compute(ms);

    ASSERT_EQ(table.size(), 3u);
    EXPECT_EQ(table[0].TeamId, "T1");
    EXPECT_EQ(table[0].Wins, 1);
    EXPECT_EQ(table[0].PointsFor, 15);
    EXPECT_EQ(table[0].PointsAgainst, 5);

    EXPECT_EQ(table[1].TeamId, "T3");
    EXPECT_EQ(table[2].TeamId, "T2");
}

TEST(Standings, Top8) {
    std::vector<Row> table;
    for (int i=0;i<10;++i) {
        Row r;
        r.TeamId = "T" + std::to_string(i+1);
        r.Wins = 10 - i;
        table.push_back(r);
    }
    StandingsCalculator calc;
    auto ids = calc.TopN(table, 8);
    ASSERT_EQ(ids.size(), 8u);
    EXPECT_EQ(ids.front(), "T1");
    EXPECT_EQ(ids.back(), "T8");
}
