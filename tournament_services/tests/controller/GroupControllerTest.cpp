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
    MOCK_METHOD(std::optional<std::string>, CreateGroup, (std::string_view tournamentId, const domain::Group& group, std::string& outGroupId), (override));
    MOCK_METHOD(std::optional<std::string>, GetGroups, (std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups), (override));
    MOCK_METHOD(std::optional<std::string>, GetGroup, (std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup), (override));
    MOCK_METHOD(std::optional<std::string>, UpdateGroup, (std::string_view tournamentId, const domain::Group& group), (override));
    MOCK_METHOD(std::optional<std::string>, RemoveGroup, (std::string_view tournamentId, std::string_view groupId), (override));
    MOCK_METHOD(std::optional<std::string>, UpdateTeams, (std::string_view tournamentId, std::string_view groupId, const std::vector<domain::Team>& teams), (override));
};

TEST(GroupController, ListGroups_200) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);
    std::vector<std::shared_ptr<domain::Group>> groups;

    EXPECT_CALL(*mock, GetGroups(_, _))
        .WillOnce(Return(std::nullopt));

    crow::request req;
    auto res = ctrl.GetGroups("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");

    EXPECT_EQ(res.code, crow::status::OK);
    EXPECT_EQ(res.get_header_value("content-type"), "application/json");
}

TEST(GroupController, CreateGroup_201_and_409) {
    auto mock = std::make_shared<GroupDelegateMock>();
    GroupController ctrl(mock);

    crow::request okReq;
    okReq.body = R"({"name":"G1","teams":[]})";
    std::string outGroupId;

    EXPECT_CALL(*mock, CreateGroup(_, _, _))
        .WillOnce(Return(std::nullopt));

    auto ok = ctrl.CreateGroup(okReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(ok.code, crow::status::CREATED);

    crow::request dupReq;
    dupReq.body = R"({"name":"dup"})";

    EXPECT_CALL(*mock, CreateGroup(_, _, _))
        .WillOnce(Return(std::make_optional<std::string>("duplicate_group_name")));

    auto dup = ctrl.CreateGroup(dupReq, "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    EXPECT_EQ(dup.code, crow::status::CONFLICT);
}
