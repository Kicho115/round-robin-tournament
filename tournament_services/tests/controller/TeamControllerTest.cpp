// tournament_services/tests/controller/TeamControllerTest.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <optional>
#include <nlohmann/json.hpp>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;

class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string& id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(std::optional<std::string>, UpdateTeam, (const domain::Team&), (const, override));
};

class TeamControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController   = std::make_shared<TeamController>(teamDelegateMock);
    }
};

TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
    crow::response badRequest = teamController->getTeam("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetTeamById) {
    auto expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id", "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(Eq(std::string("my-id"))))
        .WillOnce(Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id,   jsonResponse.at("id").get<std::string>());
    EXPECT_EQ(expectedTeam->Name, jsonResponse.at("name").get<std::string>());
}

TEST_F(TeamControllerTest, GetTeamNotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(Eq(std::string("my-id"))))
        .WillOnce(Return(nullptr));

    crow::response response = teamController->getTeam("my-id");
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, SaveTeam_Created_201) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamDelegateMock, SaveTeam(_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedTeam),
            Return(std::string("new-id"))
        ));

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(),   capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
    EXPECT_EQ(response.get_header_value("location"), "new-id");
}

TEST_F(TeamControllerTest, CreateTeam_Duplicate_Returns409) {
    // Simulamos que el delegate regresa "" como ID -> duplicado/conflicto
    EXPECT_CALL(*teamDelegateMock, SaveTeam(_))
        .WillOnce(Return(std::string{}));

    crow::request req;
    req.body = R"({"name":"TEAM NAME"})";

    auto resp = teamController->SaveTeam(req);
    EXPECT_EQ(resp.code, crow::CONFLICT);
}

TEST_F(TeamControllerTest, GetAllTeams_200_WithList) {
    auto t1 = std::make_shared<domain::Team>();
    t1->Id   = "id1";
    t1->Name = "A";

    auto t2 = std::make_shared<domain::Team>();
    t2->Id   = "id2";
    t2->Name = "B";

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{t1, t2}));

    auto resp = teamController->getAllTeams();
    EXPECT_EQ(resp.code, crow::OK);

    auto j = nlohmann::json::parse(resp.body);
    ASSERT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2);

    // Verificación básica de contenido
    std::vector<std::string> ids;
    for (auto& e : j) ids.push_back(e.at("id").get<std::string>());
    EXPECT_THAT(ids, ::testing::UnorderedElementsAre("id1", "id2"));
}

TEST_F(TeamControllerTest, GetAllTeams_200_Empty) {
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    auto resp = teamController->getAllTeams();
    EXPECT_EQ(resp.code, crow::OK);

    auto j = nlohmann::json::parse(resp.body);
    ASSERT_TRUE(j.is_array());
    EXPECT_TRUE(j.empty());
}

// Suite separada para PATCH (puede ser TEST normal porque es OTRO test suite)
TEST(TeamControllerPatchTest, UpdateTeam_204_and_404) {
    auto mock = std::make_shared<TeamDelegateMock>();
    TeamController ctrl(mock);

    nlohmann::json body = { {"name","updated"} };
    crow::request req204; req204.body = body.dump();

    EXPECT_CALL(*mock, UpdateTeam(::testing::_))
        .WillOnce(Return(std::nullopt));
    auto ok = ctrl.UpdateTeam(req204, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(crow::NO_CONTENT, ok.code);

    crow::request req404; req404.body = body.dump();
    EXPECT_CALL(*mock, UpdateTeam(::testing::_))
        .WillOnce(Return(std::make_optional<std::string>("team_not_found")));
    auto nf = ctrl.UpdateTeam(req404, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(crow::NOT_FOUND, nf.code);
}
