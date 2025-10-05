#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
    public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
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
                testing::Return("new-id")
            )
        );

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}
// ... includes y fixture existentes

TEST_F(TeamControllerTest, SaveTeam_Conflict409_WhenDuplicate) {
    // Simula que el delegate retorna id vacío para mapear 409
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::Return(std::string_view{}));

    nlohmann::json body = { {"name","dup team"} };
    crow::request req; req.body = body.dump();
    auto res = teamController->SaveTeam(req);

    EXPECT_EQ(crow::CONFLICT, res.code);
}

TEST_F(TeamControllerTest, GetAllTeams_EmptyAndNonEmpty) {
    // vacío
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Team>>{}));
    auto emptyRes = teamController->getAllTeams();
    EXPECT_EQ(200, emptyRes.code);

    // no vacío
    std::vector<std::shared_ptr<domain::Team>> list;
    list.push_back(std::make_shared<domain::Team>(domain::Team{"id1","A"}));
    list.push_back(std::make_shared<domain::Team>(domain::Team{"id2","B"}));
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(list));
    auto res = teamController->getAllTeams();
    EXPECT_EQ(200, res.code);
}

class ITeamDelegatePatchMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(std::expected<void,std::string>, UpdateTeam, (const domain::Team&), (const, override));
};

TEST(TeamControllerPatchTest, UpdateTeam_204_and_404) {
    auto mock = std::make_shared<ITeamDelegatePatchMock>();
    TeamController ctrl(mock);

    // 204
    nlohmann::json body = { {"name","updated"} };
    crow::request req204; req204.body = body.dump();
    EXPECT_CALL(*mock, UpdateTeam(::testing::_))
        .WillOnce(testing::Return(std::expected<void,std::string>{}));
    auto ok = ctrl.UpdateTeam(req204, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(crow::NO_CONTENT, ok.code);

    // 404
    crow::request req404; req404.body = body.dump();
    EXPECT_CALL(*mock, UpdateTeam(::testing::_))
        .WillOnce(testing::Return(std::unexpected("team_not_found")));
    auto nf = ctrl.UpdateTeam(req404, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(crow::NOT_FOUND, nf.code);
}
