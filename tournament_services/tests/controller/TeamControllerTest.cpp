#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <optional>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string& id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(std::optional<std::string>, UpdateTeam, (const domain::Team&), (const, override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }
};

TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
    crow::response badRequest = teamController->getTeam("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetTeamById) {
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id",  "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

TEST_F(TeamControllerTest, GetTeamNotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam("my-id");
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, SaveTeamTest) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return(std::string("new-id"))
        ));

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);
    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

TEST(TeamControllerPatchTest, UpdateTeam_204_and_404) {
    auto mock = std::make_shared<TeamDelegateMock>();
    TeamController ctrl(mock);

    nlohmann::json body = { {"name","updated"} };
    crow::request req204; req204.body = body.dump();
    EXPECT_CALL(*mock, UpdateTeam(::testing::_))
        .WillOnce(testing::Return(std::nullopt));
    auto ok = ctrl.UpdateTeam(req204, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(crow::NO_CONTENT, ok.code);

    crow::request req404; req404.body = body.dump();
    EXPECT_CALL(*mock, UpdateTeam(::testing::_))
        .WillOnce(testing::Return(std::make_optional<std::string>("team_not_found")));
    auto nf = ctrl.UpdateTeam(req404, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(crow::NOT_FOUND, nf.code);
}
