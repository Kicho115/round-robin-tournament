#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "consumer/MatchGenerationConsumer.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"

// Mocks
class MockGroupRepository : public IGroupRepository {
public:
    MockGroupRepository() = default;
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, ReadAll, (), (override));
    MOCK_METHOD(std::optional<std::string>, 
                GetGroups, 
                (std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups),
                (override));
    MOCK_METHOD(std::optional<std::string>,
                GetGroup,
                (std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup),
                (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (std::string_view tournamentId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, 
                FindByTournamentIdAndGroupId, 
                (std::string_view tournamentId, std::string_view groupId),
                (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>,
                FindByTournamentIdAndTeamId,
                (std::string_view tournamentId, std::string_view teamId),
                (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (std::string_view groupId, const std::shared_ptr<domain::Team>& team), (override));
    MOCK_METHOD(bool, ExistsGroupForTournament, (std::string_view tournamentId), (override));
    MOCK_METHOD(int, CountTeamsInGroup, (std::string_view groupId), (override));
    MOCK_METHOD(int, GroupsCountForTournament, (std::string_view tournamentId), (override));
    MOCK_METHOD(std::vector<domain::Team>, GetTeamsOfGroup, (std::string_view groupId), (override));
};

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

class MatchGenerationConsumerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockGroupRepository> mockGroupRepo;
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<MatchGenerationConsumer> consumer;

    void SetUp() override {
        mockGroupRepo = std::make_shared<MockGroupRepository>();
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        
        consumer = std::make_shared<MatchGenerationConsumer>(
            mockGroupRepo,
            mockMatchRepo,
            mockTournamentRepo
        );
    }
};

// Test: Generate matches when group is complete with even number of teams
TEST_F(MatchGenerationConsumerTest, GenerateMatches_EvenTeams_CreatesMatches) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"groupId", "group-1"}
    };
    
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tourn-1";
    tournament->Format().MaxTeamsPerGroup() = 4;
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    EXPECT_CALL(*mockGroupRepo, CountTeamsInGroup("group-1"))
        .WillOnce(::testing::Return(4));
    
    EXPECT_CALL(*mockMatchRepo, FindByGroupId("group-1"))
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Match>>{}));
    
    auto group = std::make_shared<domain::Group>();
    group->Id() = "group-1";
    domain::Team team1{"team-1", "Team 1"};
    domain::Team team2{"team-2", "Team 2"};
    domain::Team team3{"team-3", "Team 3"};
    domain::Team team4{"team-4", "Team 4"};
    group->Teams() = {team1, team2, team3, team4};
    
    EXPECT_CALL(*mockGroupRepo, GetGroup("tourn-1", "group-1", ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(group),
            ::testing::Return(std::nullopt)
        ));
    
    // Should create 6 matches for 4 teams (n*(n-1)/2 = 6)
    EXPECT_CALL(*mockMatchRepo, Create(::testing::_))
        .Times(6)
        .WillRepeatedly(::testing::Return("match-id"));
    
    consumer->Handle(event);
}

// Test: Generate matches with odd number of teams
TEST_F(MatchGenerationConsumerTest, GenerateMatches_OddTeams_CreatesMatches) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"groupId", "group-1"}
    };
    
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Format().MaxTeamsPerGroup() = 3;
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    EXPECT_CALL(*mockGroupRepo, CountTeamsInGroup("group-1"))
        .WillOnce(::testing::Return(3));
    
    EXPECT_CALL(*mockMatchRepo, FindByGroupId("group-1"))
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Match>>{}));
    
    auto group = std::make_shared<domain::Group>();
    domain::Team team1{"team-1", "Team 1"};
    domain::Team team2{"team-2", "Team 2"};
    domain::Team team3{"team-3", "Team 3"};
    group->Teams() = {team1, team2, team3};
    
    EXPECT_CALL(*mockGroupRepo, GetGroup("tourn-1", "group-1", ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(group),
            ::testing::Return(std::nullopt)
        ));
    
    // Should create 3 matches for 3 teams (n*(n-1)/2 = 3)
    EXPECT_CALL(*mockMatchRepo, Create(::testing::_))
        .Times(3)
        .WillRepeatedly(::testing::Return("match-id"));
    
    consumer->Handle(event);
}

// Test: Do not generate matches if group is not complete
TEST_F(MatchGenerationConsumerTest, NoGeneration_IncompleteGroup) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"groupId", "group-1"}
    };
    
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Format().MaxTeamsPerGroup() = 4;
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    EXPECT_CALL(*mockGroupRepo, CountTeamsInGroup("group-1"))
        .WillOnce(::testing::Return(2));  // Only 2 of 4 teams
    
    // Should not attempt to find existing matches or create new ones
    EXPECT_CALL(*mockMatchRepo, FindByGroupId(::testing::_))
        .Times(0);
    EXPECT_CALL(*mockMatchRepo, Create(::testing::_))
        .Times(0);
    
    consumer->Handle(event);
}

// Test: Do not regenerate matches if they already exist
TEST_F(MatchGenerationConsumerTest, NoRegeneration_MatchesExist) {
    nlohmann::json event = {
        {"tournamentId", "tourn-1"},
        {"groupId", "group-1"}
    };
    
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Format().MaxTeamsPerGroup() = 4;
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    EXPECT_CALL(*mockGroupRepo, CountTeamsInGroup("group-1"))
        .WillOnce(::testing::Return(4));
    
    // Matches already exist
    auto existingMatch = std::make_shared<domain::Match>();
    EXPECT_CALL(*mockMatchRepo, FindByGroupId("group-1"))
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Match>>{existingMatch}));
    
    // Should not create new matches
    EXPECT_CALL(*mockMatchRepo, Create(::testing::_))
        .Times(0);
    
    consumer->Handle(event);
}

// Test: Handle missing tournament gracefully
TEST_F(MatchGenerationConsumerTest, HandlesMissingTournament) {
    nlohmann::json event = {
        {"tournamentId", "invalid-tourn"},
        {"groupId", "group-1"}
    };
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("invalid-tourn"))
        .WillOnce(::testing::Return(nullptr));
    
    // Should not throw or crash
    EXPECT_NO_THROW(consumer->Handle(event));
}

// Test: Handle malformed event gracefully
TEST_F(MatchGenerationConsumerTest, HandlesMalformedEvent) {
    nlohmann::json event = {
        {"other_field", "value"}
        // Missing tournamentId and groupId
    };
    
    // Should not throw or crash
    EXPECT_NO_THROW(consumer->Handle(event));
}


