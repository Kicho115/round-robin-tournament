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

    // Capture the tournament passed to the delegate
    std::shared_ptr<domain::Tournament> capturedTournament;
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedTournament),
            ::testing::Return(std::expected<std::string, std::string>("new-tournament-id-123"))
        ));

    // Execute
    crow::response response = tournamentController->CreateTournament(request);

    // Verify
    ::testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ("new-tournament-id-123", response.get_header_value("location"));
    
    // Verify that the Tournament object was correctly created from JSON
    ASSERT_NE(nullptr, capturedTournament);
    EXPECT_EQ("Copa Mundial 2026", capturedTournament->Name());
    EXPECT_EQ(4, capturedTournament->Format().MaxTeamsPerGroup());
    EXPECT_EQ(8, capturedTournament->Format().NumberOfGroups());
    EXPECT_EQ(domain::TournamentType::ROUND_ROBIN, capturedTournament->Format().Type());
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

// Test: Successful GetById Tournament - HTTP 200
TEST_F(TournamentControllerTest, GetById_Success_Returns200) {
    std::string tournamentId = "tournament-id-456";
    
    // Create an example tournament
    auto expectedTournament = std::make_shared<domain::Tournament>("Liga de Campeones");
    expectedTournament->Id() = tournamentId;
    expectedTournament->Format().MaxTeamsPerGroup() = 4;
    expectedTournament->Format().NumberOfGroups() = 8;
    expectedTournament->Format().Type() = domain::TournamentType::ROUND_ROBIN;

    EXPECT_CALL(*tournamentDelegateMock, ReadById(::testing::Eq(tournamentId)))
        .WillOnce(::testing::Return(expectedTournament));

    crow::request request;
    crow::response response = tournamentController->GetById(request, tournamentId);

    // Verify response code
    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));

    // Verify JSON content
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_EQ(tournamentId, jsonResponse["id"].get<std::string>());
    EXPECT_EQ("Liga de Campeones", jsonResponse["name"].get<std::string>());
    EXPECT_EQ(4, jsonResponse["format"]["maxTeamsPerGroup"].get<int>());
    EXPECT_EQ(8, jsonResponse["format"]["numberOfGroups"].get<int>());
    EXPECT_EQ("ROUND_ROBIN", jsonResponse["format"]["type"].get<std::string>());
}

// Test: GetById Tournament not found - HTTP 404
TEST_F(TournamentControllerTest, GetById_NotFound_Returns404) {
    std::string tournamentId = "non-existent-id";

    EXPECT_CALL(*tournamentDelegateMock, ReadById(::testing::Eq(tournamentId)))
        .WillOnce(::testing::Return(nullptr));

    crow::request request;
    crow::response response = tournamentController->GetById(request, tournamentId);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
    
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.contains("error"));
    EXPECT_EQ("Tournament not found", jsonResponse["error"].get<std::string>());
}

// Test: ReadAll with Tournaments list - HTTP 200
TEST_F(TournamentControllerTest, ReadAll_WithTournaments_Returns200) {
    // Create list of tournaments
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;
    
    auto tournament1 = std::make_shared<domain::Tournament>("Torneo 1");
    tournament1->Id() = "id-1";
    tournament1->Format().MaxTeamsPerGroup() = 4;
    tournament1->Format().NumberOfGroups() = 2;
    tournaments.push_back(tournament1);

    auto tournament2 = std::make_shared<domain::Tournament>("Torneo 2");
    tournament2->Id() = "id-2";
    tournament2->Format().MaxTeamsPerGroup() = 6;
    tournament2->Format().NumberOfGroups() = 4;
    tournaments.push_back(tournament2);

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(::testing::Return(tournaments));

    crow::response response = tournamentController->ReadAll();

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));

    // Verify JSON content
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(2, jsonResponse.size());
    
    EXPECT_EQ("id-1", jsonResponse[0]["id"].get<std::string>());
    EXPECT_EQ("Torneo 1", jsonResponse[0]["name"].get<std::string>());
    
    EXPECT_EQ("id-2", jsonResponse[1]["id"].get<std::string>());
    EXPECT_EQ("Torneo 2", jsonResponse[1]["name"].get<std::string>());
}

// Test: ReadAll with empty list - HTTP 200
TEST_F(TournamentControllerTest, ReadAll_EmptyList_Returns200) {
    std::vector<std::shared_ptr<domain::Tournament>> emptyTournaments;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(::testing::Return(emptyTournaments));

    crow::response response = tournamentController->ReadAll();

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));

    // Verify that the response is an empty array
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(0, jsonResponse.size());
}

// Test: Successful UpdateTournament - HTTP 204
TEST_F(TournamentControllerTest, UpdateTournament_Success_Returns204) {
    std::string tournamentId = "tournament-id-789";
    
    // Simulate existing tournament
    auto existingTournament = std::make_shared<domain::Tournament>("Test Tournament");
    existingTournament->Id() = tournamentId;
    existingTournament->Format().MaxTeamsPerGroup() = 4;
    existingTournament->Format().NumberOfGroups() = 2;

    // JSON with updates
    nlohmann::json updatesJson = {
        {"name", "Test Tournament"}
    };

    crow::request request;
    request.body = updatesJson.dump();

    // Capture the updated tournament passed to UpdateTournament
    std::shared_ptr<domain::Tournament> capturedTournament;
    std::string capturedId;

    EXPECT_CALL(*tournamentDelegateMock, ReadById(::testing::Eq(tournamentId)))
        .WillOnce(::testing::Return(existingTournament));

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::SaveArg<1>(&capturedTournament),
            ::testing::Return(std::expected<std::string, std::string>(tournamentId))
        ));

    // Execute
    crow::response response = tournamentController->UpdateTournament(request, tournamentId);

    // Verify
    ::testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
    
    // Verify that the correct ID was passed
    EXPECT_EQ(tournamentId, capturedId);
    
    // Verify that the tournament was updated correctly
    ASSERT_NE(nullptr, capturedTournament);
    EXPECT_EQ("Test Tournament", capturedTournament->Name());
    // Other fields should remain unchanged
    EXPECT_EQ(4, capturedTournament->Format().MaxTeamsPerGroup());
    EXPECT_EQ(2, capturedTournament->Format().NumberOfGroups());
}

// Test: UpdateTournament ID not found - HTTP 404
TEST_F(TournamentControllerTest, UpdateTournament_NotFound_Returns404) {
    std::string tournamentId = "non-existent-tournament-id";

    nlohmann::json updatesJson = {
        {"name", "Test Tournament"}
    };

    crow::request request;
    request.body = updatesJson.dump();

    // Simulate that the tournament doesn't exist
    EXPECT_CALL(*tournamentDelegateMock, ReadById(::testing::Eq(tournamentId)))
        .WillOnce(::testing::Return(nullptr));

    // UpdateTournament should not be called if ReadById returns nullptr
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .Times(0);

    // Execute
    crow::response response = tournamentController->UpdateTournament(request, tournamentId);

    // Verify
    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
    
    auto jsonResponse = nlohmann::json::parse(response.body);
    EXPECT_TRUE(jsonResponse.contains("error"));
    EXPECT_EQ("Tournament not found", jsonResponse["error"].get<std::string>());
}
