#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/IMatchDelegate.hpp"
#include "controller/MatchController.hpp"

class MatchDelegateMock : public IMatchDelegate {
public:
    MOCK_METHOD(std::optional<std::string>, 
                GetMatches, 
                (std::string_view tournamentId, MatchFilterType filter, std::vector<MatchDTO>& outMatches),
                (override));
    
    MOCK_METHOD(std::optional<std::string>,
                GetMatch,
                (std::string_view tournamentId, std::string_view matchId, MatchDTO& outMatch),
                (override));
    
    MOCK_METHOD(std::optional<std::string>,
                UpdateScore,
                (std::string_view tournamentId, std::string_view matchId, int homeScore, int awayScore),
                (override));
};

class MatchControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchDelegateMock> matchDelegateMock;
    std::shared_ptr<MatchController> matchController;

    void SetUp() override {
        matchDelegateMock = std::make_shared<MatchDelegateMock>();
        matchController = std::make_shared<MatchController>(matchDelegateMock);
    }

    void TearDown() override {
    }
};

// Test: GET /matches - Success with results - HTTP 200
TEST_F(MatchControllerTest, GetMatches_Success_Returns200) {
    crow::request request;
    
    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, MatchFilterType::All, ::testing::_))
        .WillOnce(::testing::Invoke([](std::string_view, MatchFilterType, std::vector<MatchDTO>& outMatches) {
            MatchDTO match1;
            match1.matchId = "match-1";
            match1.tournamentId = "tourn-1";
            match1.groupId = "group-1";
            match1.home.id = "team-1";
            match1.home.name = "Team One";
            match1.visitor.id = "team-2";
            match1.visitor.name = "Team Two";
            match1.round = "regular";
            
            MatchDTO::ScoreInfo score;
            score.home = 1;
            score.visitor = 2;
            match1.score = score;
            
            outMatches.push_back(match1);
            return std::nullopt;
        }));
    
    auto response = matchController->GetMatches(request, "tourn-1");
    
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value("content-type"), "application/json");
    
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(jsonResponse.size(), 1);
    EXPECT_EQ(jsonResponse[0]["home"]["id"], "team-1");
    EXPECT_EQ(jsonResponse[0]["home"]["name"], "Team One");
    EXPECT_EQ(jsonResponse[0]["visitor"]["id"], "team-2");
    EXPECT_EQ(jsonResponse[0]["score"]["home"], 1);
    EXPECT_EQ(jsonResponse[0]["score"]["visitor"], 2);
}

// Test: GET /matches - Empty array - HTTP 200
TEST_F(MatchControllerTest, GetMatches_Empty_Returns200) {
    crow::request request;
    
    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, MatchFilterType::All, ::testing::_))
        .WillOnce(::testing::Invoke([](std::string_view, MatchFilterType, std::vector<MatchDTO>& outMatches) {
            outMatches.clear();
            return std::nullopt;
        }));
    
    auto response = matchController->GetMatches(request, "tourn-1");
    
    EXPECT_EQ(response.code, crow::OK);
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(jsonResponse.size(), 0);
}

// Test: GET /matches - Tournament not found - HTTP 404
TEST_F(MatchControllerTest, GetMatches_TournamentNotFound_Returns404) {
    crow::request request;
    
    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, MatchFilterType::All, ::testing::_))
        .WillOnce(::testing::Return(std::optional<std::string>("tournament_not_found")));
    
    auto response = matchController->GetMatches(request, "invalid-tourn");
    
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}

// Test: GET /matches?showMatches=played - Filter by played - HTTP 200
TEST_F(MatchControllerTest, GetMatches_FilterPlayed_Returns200) {
    crow::request request;
    request.url = "/tournaments/tourn-1/matches?showMatches=played";
    request.url_params = crow::query_string("?showMatches=played");
    
    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, MatchFilterType::Played, ::testing::_))
        .WillOnce(::testing::Invoke([](std::string_view, MatchFilterType, std::vector<MatchDTO>& outMatches) {
            MatchDTO match1;
            match1.matchId = "match-1";
            match1.home.id = "team-1";
            match1.home.name = "Team One";
            match1.visitor.id = "team-2";
            match1.visitor.name = "Team Two";
            match1.round = "regular";
            MatchDTO::ScoreInfo score{1, 2};
            match1.score = score;
            outMatches.push_back(match1);
            return std::nullopt;
        }));
    
    auto response = matchController->GetMatches(request, "tourn-1");
    
    EXPECT_EQ(response.code, crow::OK);
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_EQ(jsonResponse.size(), 1);
    EXPECT_TRUE(jsonResponse[0].contains("score"));
}

// Test: GET /matches?showMatches=pending - Filter by pending - HTTP 200
TEST_F(MatchControllerTest, GetMatches_FilterPending_Returns200) {
    crow::request request;
    request.url = "/tournaments/tourn-1/matches?showMatches=pending";
    request.url_params = crow::query_string("?showMatches=pending");
    
    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, MatchFilterType::Pending, ::testing::_))
        .WillOnce(::testing::Invoke([](std::string_view, MatchFilterType, std::vector<MatchDTO>& outMatches) {
            MatchDTO match1;
            match1.matchId = "match-1";
            match1.home.id = "team-1";
            match1.home.name = "Team One";
            match1.visitor.id = "team-2";
            match1.visitor.name = "Team Two";
            match1.round = "regular";
            // No score
            outMatches.push_back(match1);
            return std::nullopt;
        }));
    
    auto response = matchController->GetMatches(request, "tourn-1");
    
    EXPECT_EQ(response.code, crow::OK);
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_EQ(jsonResponse.size(), 1);
    EXPECT_FALSE(jsonResponse[0].contains("score"));
}

// Test: GET /matches/<id> - Success - HTTP 200
TEST_F(MatchControllerTest, GetMatch_Success_Returns200) {
    EXPECT_CALL(*matchDelegateMock, GetMatch(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](std::string_view, std::string_view, MatchDTO& outMatch) {
            outMatch.matchId = "match-1";
            outMatch.home.id = "team-1";
            outMatch.home.name = "Team One";
            outMatch.visitor.id = "team-2";
            outMatch.visitor.name = "Team Two";
            outMatch.round = "regular";
            MatchDTO::ScoreInfo score{1, 2};
            outMatch.score = score;
            return std::nullopt;
        }));
    
    auto response = matchController->GetMatch("tourn-1", "match-1");
    
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value("content-type"), "application/json");
    
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_EQ(jsonResponse["home"]["id"], "team-1");
    EXPECT_EQ(jsonResponse["home"]["name"], "Team One");
    EXPECT_EQ(jsonResponse["score"]["home"], 1);
}

// Test: GET /matches/<id> - Match not found - HTTP 404
TEST_F(MatchControllerTest, GetMatch_NotFound_Returns404) {
    EXPECT_CALL(*matchDelegateMock, GetMatch(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(std::optional<std::string>("match_not_found")));
    
    auto response = matchController->GetMatch("tourn-1", "invalid-match");
    
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}

// Test: PATCH /matches/<id> - Success - HTTP 204
TEST_F(MatchControllerTest, UpdateScore_Success_Returns204) {
    nlohmann::json body = {
        {"score", {
            {"home", 3},
            {"visitor", 1}
        }}
    };
    
    crow::request request;
    request.body = body.dump();
    
    EXPECT_CALL(*matchDelegateMock, UpdateScore(::testing::_, ::testing::_, 3, 1))
        .WillOnce(::testing::Return(std::nullopt));
    
    auto response = matchController->UpdateMatchScore(request, "tourn-1", "match-1");
    
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

// Test: PATCH /matches/<id> - Match not found - HTTP 404
TEST_F(MatchControllerTest, UpdateScore_NotFound_Returns404) {
    nlohmann::json body = {
        {"score", {
            {"home", 2},
            {"visitor", 0}
        }}
    };
    
    crow::request request;
    request.body = body.dump();
    
    EXPECT_CALL(*matchDelegateMock, UpdateScore(::testing::_, ::testing::_, 2, 0))
        .WillOnce(::testing::Return(std::optional<std::string>("match_not_found")));
    
    auto response = matchController->UpdateMatchScore(request, "tourn-1", "invalid-match");
    
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}

// Test: PATCH /matches/<id> - Invalid score (negative) - HTTP 422
TEST_F(MatchControllerTest, UpdateScore_NegativeScore_Returns422) {
    nlohmann::json body = {
        {"score", {
            {"home", -1},
            {"visitor", 2}
        }}
    };
    
    crow::request request;
    request.body = body.dump();
    
    EXPECT_CALL(*matchDelegateMock, UpdateScore(::testing::_, ::testing::_, -1, 2))
        .WillOnce(::testing::Return(std::optional<std::string>("invalid_score")));
    
    auto response = matchController->UpdateMatchScore(request, "tourn-1", "match-1");
    
    EXPECT_EQ(response.code, 422);  // Unprocessable Entity
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.contains("error"));
}

// Test: PATCH /matches/<id> - Invalid JSON - HTTP 422
TEST_F(MatchControllerTest, UpdateScore_InvalidJSON_Returns422) {
    crow::request request;
    request.body = "invalid json{";
    
    auto response = matchController->UpdateMatchScore(request, "tourn-1", "match-1");
    
    EXPECT_EQ(response.code, 422);  // Unprocessable Entity
}

// Test: PATCH /matches/<id> - Missing score field - HTTP 422
TEST_F(MatchControllerTest, UpdateScore_MissingScore_Returns422) {
    nlohmann::json body = {
        {"other_field", "value"}
    };
    
    crow::request request;
    request.body = body.dump();
    
    auto response = matchController->UpdateMatchScore(request, "tourn-1", "match-1");
    
    EXPECT_EQ(response.code, 422);  // Unprocessable Entity
}

// Test: PATCH /matches/<id> - Database error - HTTP 500
TEST_F(MatchControllerTest, UpdateScore_DatabaseError_Returns500) {
    nlohmann::json body = {
        {"score", {
            {"home", 1},
            {"visitor", 1}
        }}
    };
    
    crow::request request;
    request.body = body.dump();
    
    EXPECT_CALL(*matchDelegateMock, UpdateScore(::testing::_, ::testing::_, 1, 1))
        .WillOnce(::testing::Return(std::optional<std::string>("database_error")));
    
    auto response = matchController->UpdateMatchScore(request, "tourn-1", "match-1");
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
}


