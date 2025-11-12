#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "delegate/MatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/ITeamRepository.hpp"
#include "persistence/repository/ITournamentRepository.hpp"
#include "messaging/IEventBus.hpp"
#include "domain/Match.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"

// Mocks
class MockMatchRepository : public IMatchRepository {
public:
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

class MockTeamRepository : public ITeamRepository {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
};

class MockTournamentRepository : public ITournamentRepository {
public:
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
};

class MockEventBus : public IEventBus {
public:
    MOCK_METHOD(void, Publish, (std::string_view topic, const nlohmann::json& payload), (override));
};

class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockTeamRepository> mockTeamRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<MockEventBus> mockEventBus;
    std::shared_ptr<MatchDelegate> matchDelegate;

    void SetUp() override {
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockTeamRepo = std::make_shared<MockTeamRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        mockEventBus = std::make_shared<MockEventBus>();
        
        matchDelegate = std::make_shared<MatchDelegate>(
            mockMatchRepo,
            mockTeamRepo,
            mockTournamentRepo,
            mockEventBus
        );
    }
};

// Test: GetMatches - Success with team name resolution
TEST_F(MatchDelegateTest, GetMatches_Success_ReturnsMatchesWithTeamNames) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tourn-1";
    
    auto match1 = std::make_shared<domain::Match>();
    match1->Id = "match-1";
    match1->TournamentId = "tourn-1";
    match1->GroupId = "group-1";
    match1->HomeTeamId = "team-1";
    match1->AwayTeamId = "team-2";
    match1->Phase = domain::MatchPhase::RoundRobin;
    match1->Round = 1;
    match1->SetScore(2, 1);
    
    auto homeTeam = std::make_shared<domain::Team>();
    homeTeam->Id = "team-1";
    homeTeam->Name = "Home Team";
    
    auto awayTeam = std::make_shared<domain::Team>();
    awayTeam->Id = "team-2";
    awayTeam->Name = "Away Team";
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentId("tourn-1", MatchFilter::All))
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Match>>{match1}));
    
    EXPECT_CALL(*mockTeamRepo, ReadById("team-1"))
        .WillOnce(::testing::Return(homeTeam));
    
    EXPECT_CALL(*mockTeamRepo, ReadById("team-2"))
        .WillOnce(::testing::Return(awayTeam));
    
    std::vector<MatchDTO> matches;
    auto error = matchDelegate->GetMatches("tourn-1", MatchFilterType::All, matches);
    
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].home.id, "team-1");
    EXPECT_EQ(matches[0].home.name, "Home Team");
    EXPECT_EQ(matches[0].visitor.id, "team-2");
    EXPECT_EQ(matches[0].visitor.name, "Away Team");
    EXPECT_EQ(matches[0].round, "regular");
    EXPECT_TRUE(matches[0].score.has_value());
    EXPECT_EQ(matches[0].score->home, 2);
    EXPECT_EQ(matches[0].score->visitor, 1);
}

// Test: GetMatches - Tournament not found
TEST_F(MatchDelegateTest, GetMatches_TournamentNotFound_ReturnsError) {
    EXPECT_CALL(*mockTournamentRepo, ReadById("invalid-tourn"))
        .WillOnce(::testing::Return(nullptr));
    
    std::vector<MatchDTO> matches;
    auto error = matchDelegate->GetMatches("invalid-tourn", MatchFilterType::All, matches);
    
    EXPECT_TRUE(error.has_value());
    EXPECT_EQ(*error, "tournament_not_found");
}

// Test: GetMatches - Filter by played matches
TEST_F(MatchDelegateTest, GetMatches_FilterPlayed_ReturnsPlayedMatches) {
    auto tournament = std::make_shared<domain::Tournament>("Tournament 1");
    tournament->Id() = "tourn-1";
    
    auto match1 = std::make_shared<domain::Match>();
    match1->Id = "match-1";
    match1->HomeTeamId = "team-1";
    match1->AwayTeamId = "team-2";
    match1->SetScore(1, 0);
    
    auto homeTeam = std::make_shared<domain::Team>();
    homeTeam->Id = "team-1";
    homeTeam->Name = "Team 1";
    
    auto awayTeam = std::make_shared<domain::Team>();
    awayTeam->Id = "team-2";
    awayTeam->Name = "Team 2";
    
    EXPECT_CALL(*mockTournamentRepo, ReadById("tourn-1"))
        .WillOnce(::testing::Return(tournament));
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentId("tourn-1", MatchFilter::Played))
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Match>>{match1}));
    
    EXPECT_CALL(*mockTeamRepo, ReadById("team-1"))
        .WillOnce(::testing::Return(homeTeam));
    
    EXPECT_CALL(*mockTeamRepo, ReadById("team-2"))
        .WillOnce(::testing::Return(awayTeam));
    
    std::vector<MatchDTO> matches;
    auto error = matchDelegate->GetMatches("tourn-1", MatchFilterType::Played, matches);
    
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(matches.size(), 1);
    EXPECT_TRUE(matches[0].score.has_value());
}

// Test: GetMatch - Success
TEST_F(MatchDelegateTest, GetMatch_Success_ReturnsMatch) {
    auto match = std::make_shared<domain::Match>();
    match->Id = "match-1";
    match->HomeTeamId = "team-1";
    match->AwayTeamId = "team-2";
    
    auto homeTeam = std::make_shared<domain::Team>();
    homeTeam->Id = "team-1";
    homeTeam->Name = "Home Team";
    
    auto awayTeam = std::make_shared<domain::Team>();
    awayTeam->Id = "team-2";
    awayTeam->Name = "Away Team";
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-1"))
        .WillOnce(::testing::Return(match));
    
    EXPECT_CALL(*mockTeamRepo, ReadById("team-1"))
        .WillOnce(::testing::Return(homeTeam));
    
    EXPECT_CALL(*mockTeamRepo, ReadById("team-2"))
        .WillOnce(::testing::Return(awayTeam));
    
    MatchDTO matchDto;
    auto error = matchDelegate->GetMatch("tourn-1", "match-1", matchDto);
    
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(matchDto.matchId, "match-1");
    EXPECT_EQ(matchDto.home.name, "Home Team");
    EXPECT_EQ(matchDto.visitor.name, "Away Team");
}

// Test: GetMatch - Match not found
TEST_F(MatchDelegateTest, GetMatch_NotFound_ReturnsError) {
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "invalid-match"))
        .WillOnce(::testing::Return(nullptr));
    
    MatchDTO matchDto;
    auto error = matchDelegate->GetMatch("tourn-1", "invalid-match", matchDto);
    
    EXPECT_TRUE(error.has_value());
    EXPECT_EQ(*error, "match_not_found");
}

// Test: UpdateScore - Success with valid scores
TEST_F(MatchDelegateTest, UpdateScore_ValidScores_Success) {
    auto match = std::make_shared<domain::Match>();
    match->Id = "match-1";
    match->TournamentId = "tourn-1";
    match->GroupId = "group-1";
    match->HomeTeamId = "team-1";
    match->AwayTeamId = "team-2";
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-1"))
        .WillOnce(::testing::Return(match));
    
    EXPECT_CALL(*mockMatchRepo, UpdateScore("match-1", 3, 2))
        .WillOnce(::testing::Return(true));
    
    EXPECT_CALL(*mockEventBus, Publish("match.score_updated", ::testing::_))
        .Times(1);
    
    auto error = matchDelegate->UpdateScore("tourn-1", "match-1", 3, 2);
    
    EXPECT_FALSE(error.has_value());
}

// Test: UpdateScore - Reject negative home score
TEST_F(MatchDelegateTest, UpdateScore_NegativeHomeScore_ReturnsError) {
    auto error = matchDelegate->UpdateScore("tourn-1", "match-1", -1, 2);
    
    EXPECT_TRUE(error.has_value());
    EXPECT_EQ(*error, "invalid_score");
}

// Test: UpdateScore - Reject negative away score
TEST_F(MatchDelegateTest, UpdateScore_NegativeAwayScore_ReturnsError) {
    auto error = matchDelegate->UpdateScore("tourn-1", "match-1", 2, -1);
    
    EXPECT_TRUE(error.has_value());
    EXPECT_EQ(*error, "invalid_score");
}

// Test: UpdateScore - Accept zero scores
TEST_F(MatchDelegateTest, UpdateScore_ZeroScores_Success) {
    auto match = std::make_shared<domain::Match>();
    match->Id = "match-1";
    match->GroupId = "group-1";
    match->HomeTeamId = "team-1";
    match->AwayTeamId = "team-2";
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-1"))
        .WillOnce(::testing::Return(match));
    
    EXPECT_CALL(*mockMatchRepo, UpdateScore("match-1", 0, 0))
        .WillOnce(::testing::Return(true));
    
    EXPECT_CALL(*mockEventBus, Publish(::testing::_, ::testing::_))
        .Times(1);
    
    auto error = matchDelegate->UpdateScore("tourn-1", "match-1", 0, 0);
    
    EXPECT_FALSE(error.has_value());
}

// Test: UpdateScore - Match not found
TEST_F(MatchDelegateTest, UpdateScore_MatchNotFound_ReturnsError) {
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "invalid-match"))
        .WillOnce(::testing::Return(nullptr));
    
    auto error = matchDelegate->UpdateScore("tourn-1", "invalid-match", 1, 1);
    
    EXPECT_TRUE(error.has_value());
    EXPECT_EQ(*error, "match_not_found");
}

// Test: UpdateScore - Database error
TEST_F(MatchDelegateTest, UpdateScore_DatabaseError_ReturnsError) {
    auto match = std::make_shared<domain::Match>();
    match->Id = "match-1";
    match->GroupId = "group-1";
    match->HomeTeamId = "team-1";
    match->AwayTeamId = "team-2";
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-1"))
        .WillOnce(::testing::Return(match));
    
    EXPECT_CALL(*mockMatchRepo, UpdateScore("match-1", 1, 1))
        .WillOnce(::testing::Return(false));
    
    auto error = matchDelegate->UpdateScore("tourn-1", "match-1", 1, 1);
    
    EXPECT_TRUE(error.has_value());
    EXPECT_EQ(*error, "database_error");
}

// Test: UpdateScore - Event published with correct data
TEST_F(MatchDelegateTest, UpdateScore_PublishesEventWithCorrectData) {
    auto match = std::make_shared<domain::Match>();
    match->Id = "match-1";
    match->TournamentId = "tourn-1";
    match->GroupId = "group-1";
    match->HomeTeamId = "team-1";
    match->AwayTeamId = "team-2";
    
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndMatchId("tourn-1", "match-1"))
        .WillOnce(::testing::Return(match));
    
    EXPECT_CALL(*mockMatchRepo, UpdateScore("match-1", 2, 3))
        .WillOnce(::testing::Return(true));
    
    EXPECT_CALL(*mockEventBus, Publish("match.score_updated", ::testing::_))
        .WillOnce(::testing::Invoke([](std::string_view topic, const nlohmann::json& payload) {
            EXPECT_EQ(payload["event"], "match_score_updated");
            EXPECT_EQ(payload["tournamentId"], "tourn-1");
            EXPECT_EQ(payload["matchId"], "match-1");
            EXPECT_EQ(payload["groupId"], "group-1");
            EXPECT_EQ(payload["homeTeamId"], "team-1");
            EXPECT_EQ(payload["awayTeamId"], "team-2");
            EXPECT_EQ(payload["homeScore"], 2);
            EXPECT_EQ(payload["awayScore"], 3);
        }));
    
    auto error = matchDelegate->UpdateScore("tourn-1", "match-1", 2, 3);
    
    EXPECT_FALSE(error.has_value());
}


