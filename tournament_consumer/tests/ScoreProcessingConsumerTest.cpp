#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "consumer/ScoreProcessingConsumer.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"
#include "domain/Match.hpp"

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
};

// Test: Process score update when tournament is not complete
TEST_F(ScoreProcessingConsumerTest, ProcessScore_TournamentNotComplete) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-1"},
        {"homeScore", 2},
        {"awayScore", 1}
    };
    
    EXPECT_CALL(*mockMatchRepo, CountTotalMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(6));
    
    EXPECT_CALL(*mockMatchRepo, CountCompletedMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(3));  // Only 3 of 6 matches complete
    
    // Should not attempt to update tournament
    EXPECT_CALL(*mockTournamentRepo, ReadById(::testing::_))
        .Times(0);
    
    consumer->Handle(event);
}

// Test: Detect tournament completion when all matches are scored
TEST_F(ScoreProcessingConsumerTest, ProcessScore_TournamentComplete) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-6"},
        {"homeScore", 1},
        {"awayScore", 1}
    };
    
    EXPECT_CALL(*mockMatchRepo, CountTotalMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(6));
    
    EXPECT_CALL(*mockMatchRepo, CountCompletedMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(6));  // All 6 matches complete
    
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tourn-1";
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    consumer->Handle(event);
}

// Test: Handle missing tournament gracefully
TEST_F(ScoreProcessingConsumerTest, HandlesMissingTournament_WhenComplete) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-1"}
    };
    
    EXPECT_CALL(*mockMatchRepo, CountTotalMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mockMatchRepo, CountCompletedMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(1));
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(nullptr));
    
    // Should not throw or crash
    EXPECT_NO_THROW(consumer->Handle(event));
}

// Test: Handle malformed event gracefully
TEST_F(ScoreProcessingConsumerTest, HandlesMalformedEvent) {
    nlohmann::json event = {
        {"other_field", "value"}
        // Missing tournamentId and matchId
    };
    
    // Should not throw or crash
    EXPECT_NO_THROW(consumer->Handle(event));
}

// Test: Handle event with missing matchId
TEST_F(ScoreProcessingConsumerTest, HandlesMissingMatchId) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"}
        // Missing matchId
    };
    
    // Should not attempt to process
    EXPECT_CALL(*mockMatchRepo, CountTotalMatchesByTournament(::testing::_))
        .Times(0);
    
    EXPECT_NO_THROW(consumer->Handle(event));
}

// Test: Handle zero matches in tournament
TEST_F(ScoreProcessingConsumerTest, HandlesZeroMatches) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-1"}
    };
    
    EXPECT_CALL(*mockMatchRepo, CountTotalMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(0));  // No matches
    
    EXPECT_CALL(*mockMatchRepo, CountCompletedMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(0));
    
    // Should not mark as complete
    EXPECT_CALL(*mockTournamentRepo, ReadById(::testing::_))
        .Times(0);
    
    consumer->Handle(event);
}

// Test: Partial completion scenario
TEST_F(ScoreProcessingConsumerTest, PartialCompletion_NoAction) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"matchId", "match-1"}
    };
    
    EXPECT_CALL(*mockMatchRepo, CountTotalMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(10));
    
    EXPECT_CALL(*mockMatchRepo, CountCompletedMatchesByTournament("tourn-1"))
        .WillOnce(::testing::Return(7));  // 7 of 10 complete
    
    // Should not mark as complete
    EXPECT_CALL(*mockTournamentRepo, ReadById(::testing::_))
        .Times(0);
    
    consumer->Handle(event);
}


