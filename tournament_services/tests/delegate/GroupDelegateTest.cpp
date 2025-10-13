#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <string>
#include <memory>
#include <vector>

#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

// Mock de la interfaz de alto nivel (IGroupDelegate) en C++17
class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD(std::optional<std::string>,
                CreateGroup,
                (std::string_view tournamentId, const domain::Group& group, std::string& outGroupId),
                (override));

    MOCK_METHOD(std::optional<std::string>,
                GetGroups,
                (std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups),
                (override));

    MOCK_METHOD(std::optional<std::string>,
                GetGroup,
                (std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup),
                (override));

    MOCK_METHOD(std::optional<std::string>,
                UpdateGroup,
                (std::string_view tournamentId, const domain::Group& group),
                (override));

    MOCK_METHOD(std::optional<std::string>,
                RemoveGroup,
                (std::string_view tournamentId, std::string_view groupId),
                (override));

    MOCK_METHOD(std::optional<std::string>,
                UpdateTeams,
                (std::string_view tournamentId, std::string_view groupId, const std::vector<domain::Team>& teams),
                (override));
};

TEST(GroupDelegateInterfaceTest, CreateGroup_Success_SetsLocationId) {
    GroupDelegateMock mock;

    // arrange: cuando se llama CreateGroup, pone "new-group-id" en el parámetro de salida y retorna éxito (nullopt)
    EXPECT_CALL(mock, CreateGroup("t-1", _, _))
        .WillOnce(DoAll(SetArgReferee<2>(std::string("new-group-id")),
                        Return(std::nullopt)));

    std::string outId;
    domain::Group g; g.Name() = "Group A";
    auto err = mock.CreateGroup("t-1", g, outId);

    ASSERT_FALSE(err.has_value());
    EXPECT_EQ(outId, "new-group-id");
}

TEST(GroupDelegateInterfaceTest, CreateGroup_Duplicate_ReturnsConflictMessage) {
    GroupDelegateMock mock;

    EXPECT_CALL(mock, CreateGroup("t-dup", _, _))
        .WillOnce(Return(std::make_optional<std::string>("duplicate_group_name")));

    std::string outId;
    domain::Group g; g.Name() = "Group A";
    auto err = mock.CreateGroup("t-dup", g, outId);

    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "duplicate_group_name");
}

TEST(GroupDelegateInterfaceTest, GetGroup_NotFound) {
    GroupDelegateMock mock;

    std::shared_ptr<domain::Group> out;
    EXPECT_CALL(mock, GetGroup("t-1", "g-404", _))
        .WillOnce(Return(std::make_optional<std::string>("group_not_found")));

    auto err = mock.GetGroup("t-1", "g-404", out);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "group_not_found");
}

TEST(GroupDelegateInterfaceTest, UpdateTeams_Success_And_Errors) {
    GroupDelegateMock mock;

    std::vector<domain::Team> teams;
    teams.push_back(domain::Team{"id-1", "Team 1"});
    teams.push_back(domain::Team{"id-2", "Team 2"});

    // éxito
    EXPECT_CALL(mock, UpdateTeams("t-1", "g-1", teams))
        .WillOnce(Return(std::nullopt));
    auto ok = mock.UpdateTeams("t-1", "g-1", teams);
    EXPECT_FALSE(ok.has_value());

    // torneo no encontrado
    EXPECT_CALL(mock, UpdateTeams("t-404", "g-1", _))
        .WillOnce(Return(std::make_optional<std::string>("tournament_not_found")));
    auto nf = mock.UpdateTeams("t-404", "g-1", teams);
    ASSERT_TRUE(nf.has_value());
    EXPECT_EQ(*nf, "tournament_not_found");
}

TEST(GroupDelegate, CreateGroup_Success) {
    GroupDelegateMock mock;
    std::string outGroupId;
    domain::Group group{"g1", "Group 1"};
    EXPECT_CALL(mock, CreateGroup("t1", group, _))
        .WillOnce(DoAll(SetArgReferee<2>("g1"), Return(std::nullopt)));
    auto result = mock.CreateGroup("t1", group, outGroupId);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(outGroupId, "g1");
}

TEST(GroupDelegate, CreateGroup_Fails) {
    GroupDelegateMock mock;
    std::string outGroupId;
    domain::Group group{"g1", "Group 1"};
    EXPECT_CALL(mock, CreateGroup("t1", group, _))
        .WillOnce(Return(std::make_optional<std::string>("duplicate_group_name")));
    auto result = mock.CreateGroup("t1", group, outGroupId);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "duplicate_group_name");
}

TEST(GroupDelegate, GetGroups_ReturnsList) {
    GroupDelegateMock mock;
    std::vector<std::shared_ptr<domain::Group>> groups = {
        std::make_shared<domain::Group>(domain::Group{"g1", "Group 1"}),
        std::make_shared<domain::Group>(domain::Group{"g2", "Group 2"})
    };
    EXPECT_CALL(mock, GetGroups("t1", _))
        .WillOnce(DoAll(SetArgReferee<1>(groups), Return(std::nullopt)));
    std::vector<std::shared_ptr<domain::Group>> outGroups;
    auto result = mock.GetGroups("t1", outGroups);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(outGroups.size(), 2);
    EXPECT_EQ(outGroups[0]->Id(), "g1");
    EXPECT_EQ(outGroups[1]->Id(), "g2");
}

TEST(GroupDelegate, GetGroups_ReturnsEmptyList) {
    GroupDelegateMock mock;
    std::vector<std::shared_ptr<domain::Group>> groups;
    EXPECT_CALL(mock, GetGroups("t1", _))
        .WillOnce(DoAll(SetArgReferee<1>(groups), Return(std::nullopt)));
    std::vector<std::shared_ptr<domain::Group>> outGroups;
    auto result = mock.GetGroups("t1", outGroups);
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(outGroups.empty());
}

TEST(GroupDelegate, UpdateGroup_Success) {
    GroupDelegateMock mock;
    domain::Group group{"g1", "Group 1"};
    EXPECT_CALL(mock, UpdateGroup("t1", group))
        .WillOnce(Return(std::nullopt));
    auto result = mock.UpdateGroup("t1", group);
    EXPECT_FALSE(result.has_value());
}

TEST(GroupDelegate, UpdateGroup_Fails) {
    GroupDelegateMock mock;
    domain::Group group{"g1", "Group 1"};
    EXPECT_CALL(mock, UpdateGroup("t1", group))
        .WillOnce(Return(std::make_optional<std::string>("group_not_found")));
    auto result = mock.UpdateGroup("t1", group);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "group_not_found");
}
