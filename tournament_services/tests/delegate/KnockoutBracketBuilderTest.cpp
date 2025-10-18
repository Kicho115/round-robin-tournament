#include <gtest/gtest.h>
#include "delegate/KnockoutBracketBuilder.hpp"

using standings::Row;
using domain::MatchPhase;

TEST(KO8, PairingsAndCount) {
    std::vector<Row> table(8);
    for (int i=0;i<8;++i) {
        table[i].TeamId = "T" + std::to_string(i+1);
        table[i].Wins = 100 - i;
    }
    auto ko = KnockoutBracketBuilder::BuildTop8(table, "T-2025");
    ASSERT_EQ(ko.size(), 7u);

    EXPECT_EQ(ko[0].HomeTeamId, "T1"); EXPECT_EQ(ko[0].AwayTeamId, "T8");
    EXPECT_EQ(ko[1].HomeTeamId, "T4"); EXPECT_EQ(ko[1].AwayTeamId, "T5");
    EXPECT_EQ(ko[2].HomeTeamId, "T3"); EXPECT_EQ(ko[2].AwayTeamId, "T6");
    EXPECT_EQ(ko[3].HomeTeamId, "T2"); EXPECT_EQ(ko[3].AwayTeamId, "T7");
    for (int i=0;i<4;++i) {
        EXPECT_EQ(ko[i].Phase, MatchPhase::Knockout);
        EXPECT_EQ(ko[i].Round, 1);
    }

    EXPECT_EQ(ko[4].Round, 2);
    EXPECT_EQ(ko[5].Round, 2);
    EXPECT_NE(ko[4].HomeTeamId.find("W("), std::string::npos);
    EXPECT_NE(ko[5].HomeTeamId.find("W("), std::string::npos);

    EXPECT_EQ(ko[6].Round, 3);
    EXPECT_NE(ko[6].HomeTeamId.find("W("), std::string::npos);
}
