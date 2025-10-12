#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"

using ::testing::Return;
using ::testing::_;
using nlohmann::json;

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD(nlohmann::json, ReadByTournament, (const std::string&), (const, override));
    MOCK_METHOD(std::expected<std::string,std::string>, CreateGroup, (const std::string&, const std::string&, const nlohmann::json&), (const, override));
    MOCK_METHOD(std::expected<void,std::string>, AddTeamToGroup, (const std::string&, const std::string&, const std::string&, const std::string&), (const, override));
};

TEST(GroupController, ListGroups_200) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    EXPECT_CALL(*mock, ReadByTournament(_)).WillOnce(Return(json::array()));
    crow::request req;
    auto res = ctrl.GetGroups(req, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(res.code, crow::OK);
    EXPECT_EQ(res.get_header_value("content-type"), "application/json");
}

TEST(GroupController, CreateGroup_201_and_409) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    crow::request okReq; okReq.body = R"({"name":"G1","teams":[]})";
    EXPECT_CALL(*mock, CreateGroup(_,_,_)).WillOnce(Return(std::expected<std::string,std::string>{"gid-1"}));
    auto ok = ctrl.CreateGroup(okReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(ok.code, crow::CREATED);

    crow::request dupReq; dupReq.body = R"({"name":"dup"})";
    EXPECT_CALL(*mock, CreateGroup(_,_,_)).WillOnce(Return(std::unexpected("duplicate_group_name")));
    auto dup = ctrl.CreateGroup(dupReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(dup.code, crow::CONFLICT);
}

TEST(GroupController, AddTeam_201_and_422) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    crow::request okReq; okReq.body = R"({"id":"t1","name":"Team 1"})";
    EXPECT_CALL(*mock, AddTeamToGroup(_,_,_,_)).WillOnce(Return(std::expected<void,std::string>{}));
    auto ok = ctrl.AddTeam(okReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa", "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb");
    EXPECT_EQ(ok.code, crow::CREATED);

    crow::request nfReq; nfReq.body = R"({"id":"bad","name":"X"})";
    EXPECT_CALL(*mock, AddTeamToGroup(_,_,_,_)).WillOnce(Return(std::unexpected("team_not_found")));
    auto nf = ctrl.AddTeam(nfReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa", "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb");
    EXPECT_EQ(nf.code, crow::UNPROCESSABLE);
}
TEST(GroupController, CreateGroup_SecondOne_Conflict409) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    // primer create OK
    crow::request r1; r1.body = R"({"name":"Group A"})";
    EXPECT_CALL(*mock, CreateGroup(::testing::_,::testing::_,::testing::_))
      .WillOnce(::testing::Return(std::expected<std::string,std::string>{"gid-1"}));
    auto ok = ctrl.CreateGroup(r1, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(ok.code, crow::CREATED);

    // segundo create en mismo torneo → 409
    crow::request r2; r2.body = R"({"name":"Group B"})";
    EXPECT_CALL(*mock, CreateGroup(::testing::_,::testing::_,::testing::_))
      .WillOnce(::testing::Return(std::unexpected("group_limit_reached")));
    auto conflict = ctrl.CreateGroup(r2, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(conflict.code, crow::CONFLICT);
}
