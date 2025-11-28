#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "consumer/ScoreProcessingConsumer.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"
#include "domain/Match.hpp"

using namespace domain;

// Mocks
class MockMatchRepository : public IMatchRepository {
public:
    MockMatchRepository() = default;
    MOCK_METHOD(std::shared_ptr<domain::Match>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Match& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, ReadAll, (), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, 
                FindByTournamentId, 
                (std::string_view tournamentId, MatchFilter filter),
                (override));
    MOCK_METHOD(std::shared_ptr<domain::Match>,
                FindByTournamentIdAndMatchId,
                (std::string_view tournamentId, std::string_view matchId),
                (override));
    MOCK_METHOD(bool, UpdateScore, (std::string_view matchId, int homeScore, int awayScore), (override));
    MOCK_METHOD(int, CountCompletedMatchesByTournament, (std::string_view tournamentId), (override));
    MOCK_METHOD(int, CountTotalMatchesByTournament, (std::string_view tournamentId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByGroupId, (std::string_view groupId), (override));
};

class MockTournamentRepository : public IRepository<domain::Tournament, std::string> {
public:
    MockTournamentRepository() = default;
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
};

class ScoreProcessingConsumerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<ScoreProcessingConsumer> consumer;

    void SetUp() override {
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        
        consumer = std::make_shared<ScoreProcessingConsumer>(
            mockMatchRepo,
            mockTournamentRepo
        );
    }

    std::shared_ptr<Match> createMatch(const std::string& id, MatchPhase phase, int homeScore = -1, int awayScore = -1) {
        auto m = std::make_shared<Match>();
        m->Id = id;
        m->TournamentId = "tourn-1";
        m->Phase = phase;
        m->HomeTeamId = "H";
        m->AwayTeamId = "A";
        if (homeScore >= 0 && awayScore >= 0) {
            m->SetScore(homeScore, awayScore);
        }
        return m;
    }
};

// Test: Process score update when RR is not complete
TEST_F(ScoreProcessingConsumerTest, ProcessScore_RRNotComplete) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-1"},
        {"homeScore", 2},
        {"awayScore", 1}
    };
    
    auto m1 = createMatch("match-1", MatchPhase::RoundRobin, 2, 1);
    auto m2 = createMatch("match-2", MatchPhase::RoundRobin, -1, -1); // Pending
    std::vector<std::shared_ptr<Match>> matches = {m1, m2};

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-1"))
        .WillOnce(::testing::Return(m1));

    EXPECT_CALL(*mockMatchRepo, FindByTournamentId("tourn-1", MatchFilter::All))
        .WillOnce(::testing::Return(matches));
    
    // Should not generate playoffs or complete tournament
    EXPECT_CALL(*mockMatchRepo, Create(::testing::_)).Times(0);
    
    consumer->Handle(event);
}

// Test: RR Complete, Generate Playoffs
TEST_F(ScoreProcessingConsumerTest, ProcessScore_RRComplete_GeneratesPlayoffs) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-6"}
    };
    
    // 4 Teams -> 6 Matches
    std::vector<std::shared_ptr<Match>> matches;
    for(int i=0; i<6; ++i) {
        auto m = createMatch("match-"+std::to_string(i), MatchPhase::RoundRobin, 1, 0);
        m->HomeTeamId = "T" + std::to_string(i%4);
        m->AwayTeamId = "T" + std::to_string((i+1)%4);
        matches.push_back(m);
    }

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-6"))
        .WillOnce(::testing::Return(matches.back()));

    EXPECT_CALL(*mockMatchRepo, FindByTournamentId("tourn-1", MatchFilter::All))
        .WillOnce(::testing::Return(matches));
    
    // Should create at least one playoff match (Top 4 -> 2 semis + 1 final)
    // Actually BuildTop8FromSeeds will create 3 matches (2 semis, 1 final) if 4 teams
    EXPECT_CALL(*mockMatchRepo, Create(::testing::_)).Times(::testing::AtLeast(1));
    
    consumer->Handle(event);
}

// Test: KO Match, Advances Winner
TEST_F(ScoreProcessingConsumerTest, ProcessScore_KOMatch_AdvancesWinner) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "ko-1"}
    };
    
    auto mKO = createMatch("ko-1", MatchPhase::Knockout, 2, 1);
    mKO->HomeTeamId = "T1";
    mKO->AwayTeamId = "T2";
    
    auto mNext = createMatch("ko-2", MatchPhase::Knockout);
    mNext->HomeTeamId = "W(T1-T2)"; // Placeholder
    mNext->AwayTeamId = "T3";

    std::vector<std::shared_ptr<Match>> matches = {mKO, mNext};

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "ko-1"))
        .WillOnce(::testing::Return(mKO));

    // Expect 2 calls to FindByTournamentId (one for advancement, one for completion check)
    EXPECT_CALL(*mockMatchRepo, FindByTournamentId("tourn-1", MatchFilter::All))
        .Times(2)
        .WillRepeatedly(::testing::Return(matches));

    // Should update mNext with Winner T1
    EXPECT_CALL(*mockMatchRepo, Update(::testing::_)).WillOnce([&](const Match& m){
        EXPECT_EQ(m.Id, "ko-2");
        EXPECT_EQ(m.HomeTeamId, "T1");
        return "ko-2";
    });

    consumer->Handle(event);
}

// Test: All Completed
TEST_F(ScoreProcessingConsumerTest, ProcessScore_AllCompleted) {
     nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "ko-final"}
    };

    auto mKO = createMatch("ko-final", MatchPhase::Knockout, 2, 1);
    std::vector<std::shared_ptr<Match>> matches = {mKO};

     EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "ko-final"))
        .WillOnce(::testing::Return(mKO));

     // Expect 2 calls (advancement + completion)
     EXPECT_CALL(*mockMatchRepo, FindByTournamentId("tourn-1", MatchFilter::All))
        .Times(2)
        .WillRepeatedly(::testing::Return(matches));

     // Just prints for now
     consumer->Handle(event);
}

// Test: Handle malformed event gracefully
TEST_F(ScoreProcessingConsumerTest, HandlesMalformedEvent) {
    nlohmann::json event = {
        {"other_field", "value"}
        // Missing tournamentId and matchId
    };
    
    EXPECT_NO_THROW(consumer->Handle(event));
}

// Test: Handle event with missing matchId
TEST_F(ScoreProcessingConsumerTest, HandlesMissingMatchId) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"}
        // Missing matchId
    };
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId(::testing::_, ::testing::_))
        .Times(0);
    
    EXPECT_NO_THROW(consumer->Handle(event));
}
