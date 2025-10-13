#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <expected>

#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateTournament, (std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string& id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTournament, (const std::string& id, std::shared_ptr<domain::Tournament> tournament), (override));
};

class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    // Before each test
    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(tournamentDelegateMock);
    }

    // After each test
    void TearDown() override {
        // teardown code comes here
    }
};

// Test: Successful Create a Tournament - HTTP 201
TEST_F(TournamentControllerTest, CreateTournament_Success_Returns201) {
    nlohmann::json tournamentJson = {
        {"name", "Copa Mundial 2026"},
        {"format", {
            {"maxTeamsPerGroup", 4},
            {"numberOfGroups", 8},
            {"type", "ROUND_ROBIN"}
        }}
    };
    crow::request request;
    request.body = tournamentJson.dump();
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
        .WillOnce(testing::Return(std::expected<std::string, std::string>{"tourn-id-1"}));
    auto response = tournamentController->CreateTournament(request);
    EXPECT_EQ(response.code, crow::CREATED);
}

// Test: Create a Tournament with database error - HTTP 500
TEST_F(TournamentControllerTest, CreateTournament_DatabaseError_Returns500) {
    nlohmann::json tournamentJson = {
        {"name", "Torneo Duplicado"},
        {"format", {
            {"maxTeamsPerGroup", 4},
            {"numberOfGroups", 2},
            {"type", "ROUND_ROBIN"}
        }}
    };

    crow::request request;
    request.body = tournamentJson.dump();

    // Simulate database insertion error using std::expected
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(::testing::Return(std::unexpected<std::string>("Database constraint violation")));

    // Execute
    crow::response response = tournamentController->CreateTournament(request);

    // Verify
    EXPECT_EQ(crow::INTERNAL_SERVER_ERROR, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
    
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.contains("error"));
    EXPECT_EQ("Database constraint violation", jsonResponse["error"].get<std::string>());
}

// Test: Create a Tournament with duplicate name - HTTP 409
TEST_F(TournamentControllerTest, CreateTournament_Duplicate_Returns409) {
    nlohmann::json tournamentJson = {
        {"name", "Copa Mundial 2026"},
        {"format", {
            {"maxTeamsPerGroup", 4},
            {"numberOfGroups", 8},
            {"type", "ROUND_ROBIN"}
        }}
    };
    crow::request request;
    request.body = tournamentJson.dump();
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(testing::_))
        .WillOnce(testing::Return(std::unexpected("duplicate_tournament_name")));
    auto response = tournamentController->CreateTournament(request);
    EXPECT_EQ(response.code, crow::CONFLICT);
}

// Test: Successful GetById Tournament - HTTP 200
TEST_F(TournamentControllerTest, GetTournamentById_200) {
    auto tournament = std::make_shared<domain::Tournament>("Copa Mundial 2026", domain::TournamentFormat(4, 8));
    EXPECT_CALL(*tournamentDelegateMock, ReadById("tourn-id-1"))
        .WillOnce(testing::Return(tournament));
    crow::request request; // request vacío
    auto response = tournamentController->GetById(request, "tourn-id-1");
    EXPECT_EQ(response.code, crow::OK);
}

// Test: GetById Tournament not found - HTTP 404
TEST_F(TournamentControllerTest, GetTournamentById_404) {
    EXPECT_CALL(*tournamentDelegateMock, ReadById("not-found"))
        .WillOnce(testing::Return(nullptr));
    crow::request request;
    auto response = tournamentController->GetById(request, "not-found");
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}

// Test: ReadAll with Tournaments list - HTTP 200
TEST_F(TournamentControllerTest, GetAllTournaments_200) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {
        std::make_shared<domain::Tournament>("T1", domain::TournamentFormat(2, 8)),
        std::make_shared<domain::Tournament>("T2", domain::TournamentFormat(2, 8))
    };
    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));
    auto response = tournamentController->ReadAll();
    EXPECT_EQ(response.code, crow::OK);
}

// Test: Successful UpdateTournament - HTTP 204
TEST_F(TournamentControllerTest, UpdateTournament_204) {
    nlohmann::json tournamentJson = {
        {"name", "Copa Mundial 2026"},
        {"format", {
            {"maxTeamsPerGroup", 4},
            {"numberOfGroups", 8},
            {"type", "ROUND_ROBIN"}
        }}
    };
    crow::request request;
    request.body = tournamentJson.dump();
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament("tourn-id-1", testing::_))
        .WillOnce(testing::Return(std::expected<std::string, std::string>{"tourn-id-1"}));
    auto response = tournamentController->UpdateTournament(request, "tourn-id-1");
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

// Test: UpdateTournament ID not found - HTTP 404
TEST_F(TournamentControllerTest, UpdateTournament_404) {
    nlohmann::json tournamentJson = {
        {"name", "Copa Mundial 2026"},
        {"format", {
            {"maxTeamsPerGroup", 4},
            {"numberOfGroups", 8},
            {"type", "ROUND_ROBIN"}
        }}
    };
    crow::request request;
    request.body = tournamentJson.dump();
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament("not-found", testing::_))
        .WillOnce(testing::Return(std::unexpected("Tournament not found")));
    auto response = tournamentController->UpdateTournament(request, "not-found");
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}
