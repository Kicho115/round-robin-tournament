#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <expected>

#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using nlohmann::json;

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD(std::optional<std::string>, CreateGroup,
                (std::string_view tournamentId,
                 const domain::Group& group,
                 std::string& outGroupId),
                (override));

    MOCK_METHOD(std::optional<std::string>, GetGroups,
                (std::string_view tournamentId,
                 std::vector<std::shared_ptr<domain::Group>>& outGroups),
                (override));

    MOCK_METHOD(std::optional<std::string>, GetGroup,
                (std::string_view tournamentId,
                 std::string_view groupId,
                 std::shared_ptr<domain::Group>& outGroup),
                (override));

    MOCK_METHOD(std::optional<std::string>, UpdateGroup,
                (std::string_view tournamentId,
                 const domain::Group& group),
                (override));

    MOCK_METHOD(std::optional<std::string>, RemoveGroup,
                (std::string_view tournamentId,
                 std::string_view groupId),
                (override));

    MOCK_METHOD(std::optional<std::string>, UpdateTeams,
                (std::string_view tournamentId,
                 std::string_view groupId,
                 const std::vector<domain::Team>& teams),
                (override));
};

TEST(GroupController, ListGroups_200) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    // Simulamos éxito (sin error). No es necesario setear outGroups para esta aserción mínima.
    EXPECT_CALL(*mock, GetGroups(_, _))
        .WillOnce(Return(std::nullopt));

    auto res = ctrl.GetGroups("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");

    EXPECT_EQ(res.code, crow::status::OK);
    EXPECT_EQ(res.get_header_value("content-type"), "application/json");
}

TEST(GroupController, CreateGroup_201_and_409) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    // --- Caso 201 ---
    crow::request okReq;
    okReq.body = R"({"name":"G1","teams":[]})";

    EXPECT_CALL(*mock, CreateGroup(_, _, _))
        .WillOnce(DoAll(
            // Si quieres, puedes setear el outGroupId:
            SetArgReferee<2>(std::string{"g1"}),
            Return(std::nullopt)
        ));

    auto ok = ctrl.CreateGroup(okReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(ok.code, crow::status::CREATED);

    // --- Caso 409 (grupo duplicado) ---
    crow::request dupReq;
    dupReq.body = R"({"name":"dup","teams":[]})";

    EXPECT_CALL(*mock, CreateGroup(_, _, _))
        .WillOnce(Return(std::make_optional<std::string>("duplicate_group_name")));

    auto dup = ctrl.CreateGroup(dupReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(dup.code, crow::status::CONFLICT);
}

TEST(GroupController, GetGroup_200_and_404) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController controller{mock};

    // --- 200 ---
    auto g = std::make_shared<domain::Group>();
    g->Id() = "g1";
    g->TournamentId() = "t1";
    g->Name() = "Group A";

    EXPECT_CALL(*mock, GetGroup("t1", "g1", _))
        .WillOnce(DoAll(
            SetArgReferee<2>(g),
            Return(std::nullopt)
        ));

    auto ok = controller.GetGroup("t1", "g1");
    EXPECT_EQ(ok.code, crow::status::OK);
    EXPECT_EQ(ok.get_header_value("content-type"), "application/json");

    // --- 404 ---
    EXPECT_CALL(*mock, GetGroup("t1", "not-found", _))
        .WillOnce(Return(std::make_optional<std::string>("group_not_found")));

    auto nf = controller.GetGroup("t1", "not-found");
    EXPECT_EQ(nf.code, crow::status::NOT_FOUND);
}

TEST(GroupController, UpdateGroup_204_and_404) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController controller{mock};

    // --- 204 ---
    EXPECT_CALL(*mock, UpdateGroup("t1", _))
        .WillOnce(Return(std::nullopt));

    crow::request req;
    req.body = R"({"name":"Group X"})";
    auto ok = controller.UpdateGroup(req, "t1", "g1");
    EXPECT_EQ(ok.code, crow::status::NO_CONTENT);

    // --- 404 ---
    EXPECT_CALL(*mock, UpdateGroup("t1", _))
        .WillOnce(Return(std::make_optional<std::string>("group_not_found")));

    auto nf = controller.UpdateGroup(req, "t1", "nope");
    EXPECT_EQ(nf.code, crow::status::NOT_FOUND);
}

TEST(GroupController, AddTeam_201_and_422_cases) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController controller{mock};

    // --- 201 (éxito) ---
    EXPECT_CALL(*mock, UpdateTeams("t1", "g1", _))
        .WillOnce(Return(std::nullopt));

    crow::request req;
    req.body = R"({"id":"team-1","name":"Team 1"})";
    auto created = controller.AddTeam(req, "t1", "g1");
    EXPECT_EQ(created.code, crow::status::CREATED);

    // --- 422: team_not_found ---
    EXPECT_CALL(*mock, UpdateTeams("t1", "g1", _))
        .WillOnce(Return(std::make_optional<std::string>("team_not_found")));
    auto tnf = controller.AddTeam(req, "t1", "g1");
    EXPECT_EQ(tnf.code, 422);

    // --- 422: group_full ---
    EXPECT_CALL(*mock, UpdateTeams("t1", "g1", _))
        .WillOnce(Return(std::make_optional<std::string>("group_full")));
    auto full = controller.AddTeam(req, "t1", "g1");
    EXPECT_EQ(full.code, 422);
}
// GET /tournaments/<tid>/groups -> 404 si tournament_not_found
TEST(GroupController, ListGroups_404_TournamentNotFound) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    EXPECT_CALL(*mock, GetGroups(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(std::make_optional<std::string>("tournament_not_found")));

    auto res = ctrl.GetGroups("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(res.code, crow::status::NOT_FOUND);
}

// POST /tournaments/<tid>/groups -> 404 si tournament_not_found
TEST(GroupController, CreateGroup_404_TournamentNotFound) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    crow::request req;
    req.body = R"({"name":"G1","teams":[]})";

    EXPECT_CALL(*mock, CreateGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(std::make_optional<std::string>("tournament_not_found")));

    auto res = ctrl.CreateGroup(req, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(res.code, crow::status::NOT_FOUND);
}

// POST /tournaments/<tid>/groups/<gid>/teams -> 404 si tournament/group not found
TEST(GroupController, AddTeam_404_TournamentOrGroupNotFound) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController controller{mock};

    crow::request req;
    req.body = R"({"id":"team-1","name":"Team 1"})";

    // tournament_not_found
    EXPECT_CALL(*mock, UpdateTeams("t1","g1", ::testing::_))
        .WillOnce(::testing::Return(std::make_optional<std::string>("tournament_not_found")));

    auto tnf = controller.AddTeam(req, "t1", "g1");
    EXPECT_EQ(tnf.code, crow::NOT_FOUND);

    // group_not_found
    EXPECT_CALL(*mock, UpdateTeams("t1","g1", ::testing::_))
        .WillOnce(::testing::Return(std::make_optional<std::string>("group_not_found")));

    auto gnf = controller.AddTeam(req, "t1", "g1");
    EXPECT_EQ(gnf.code, crow::NOT_FOUND);
}
