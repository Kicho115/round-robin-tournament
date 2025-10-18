#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "delegate/GroupDelegate.hpp"
#include "messaging/IEventBus.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/ITournamentRepository.hpp"
#include "persistence/repository/ITeamRepository.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::Invoke;

class EventBusMock : public IEventBus {
public:
    MOCK_METHOD(void, Publish, (std::string_view, const nlohmann::json&), (override));
};

class GroupRepoMock : public IGroupRepository {
public:
    MOCK_METHOD(std::string, Create, (const domain::Group&), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, ReadAll, (), (override));

    MOCK_METHOD(std::optional<std::string>, GetGroups, (std::string_view, std::vector<std::shared_ptr<domain::Group>>&), (override));
    MOCK_METHOD(std::optional<std::string>, GetGroup, (std::string_view, std::string_view, std::shared_ptr<domain::Group>&), (override));

    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string_view&, const std::shared_ptr<domain::Team>&), (override));
    MOCK_METHOD(int, CountTeamsInGroup, (const std::string_view&), (override));
};

class TournamentRepoMock : public ITournamentRepository {
public:
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
};

class TeamRepoMock : public ITeamRepository {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Create, (const domain::Team&), (override));
    MOCK_METHOD<std::string, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
};

TEST(GroupDelegateEventsAndLimitsTest, CreateGroup_Publishes_Event) {
    auto gRepo = std::make_shared<GroupRepoMock>();
    auto tRepo = std::make_shared<TournamentRepoMock>();
    auto teamRepo = std::make_shared<TeamRepoMock>();
    auto bus = std::make_shared<EventBusMock>();

    domain::TournamentFormat fmt(1, 4, domain::TournamentType::ROUND_ROBIN);
    auto tour = std::make_shared<domain::Tournament>("T");
    tour->Id() = "t1";
    tour->Format() = fmt;

    EXPECT_CALL(*tRepo, ReadById("t1")).WillOnce(Return(tour));
    EXPECT_CALL(*gRepo, Create(::testing::_)).WillOnce(Return(std::string("g1")));
    EXPECT_CALL(*bus, Publish(::testing::StrEq("group.created"), _)).Times(1);

    GroupDelegate delegate(gRepo, tRepo, teamRepo, bus);

    domain::Group g;
    g.TournamentId() = "t1";
    g.Name() = "Group A";

    std::string outId;
    auto err = delegate.CreateGroup("t1", g, outId);
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(outId, "g1");
}

TEST(GroupDelegateEventsAndLimitsTest, AddTeam_Publishes_Event_And_Respects_Limit) {
    auto gRepo = std::make_shared<GroupRepoMock>();
    auto tRepo = std::make_shared<TournamentRepoMock>();
    auto teamRepo = std::make_shared<TeamRepoMock>();
    auto bus = std::make_shared<EventBusMock>();

    domain::TournamentFormat fmt(1, 2, domain::TournamentType::ROUND_ROBIN); // máx 2 equipos
    auto tour = std::make_shared<domain::Tournament>("T");
    tour->Id() = "t1";
    tour->Format() = fmt;

    auto group = std::make_shared<domain::Group>();
    group->Id() = "g1";
    group->TournamentId() = "t1";
    group->Name() = "G";

    EXPECT_CALL(*tRepo, ReadById("t1")).WillOnce(Return(tour));
    EXPECT_CALL(*gRepo, GetGroup("t1", "g1", ::testing::_))
        .WillOnce(DoAll(
            ::testing::Invoke([&](std::string_view, std::string_view, std::shared_ptr<domain::Group>& out){
                out = group;
            }),
            Return(std::optional<std::string>{})
        ));
    EXPECT_CALL(*gRepo, CountTeamsInGroup("g1")).WillOnce(Return(1)); // ya hay 1
    EXPECT_CALL(*teamRepo, ReadById("team-1")).WillOnce(Return(std::make_shared<domain::Team>(domain::Team{"team-1","Team 1"})));
    EXPECT_CALL(*gRepo, UpdateGroupAddTeam("g1", ::testing::_)).Times(1);
    EXPECT_CALL(*bus, Publish(::testing::StrEq("group.team_added"), _)).Times(1);

    GroupDelegate delegate(gRepo, tRepo, teamRepo, bus);

    std::vector<domain::Team> add{ domain::Team{"team-1","Team 1"} };
    auto err = delegate.UpdateTeams("t1", "g1", add);
    EXPECT_FALSE(err.has_value());
}

TEST(GroupDelegateEventsAndLimitsTest, AddTeam_GroupFull) {
    auto gRepo = std::make_shared<GroupRepoMock>();
    auto tRepo = std::make_shared<TournamentRepoMock>();
    auto teamRepo = std::make_shared<TeamRepoMock>();
    auto bus = std::make_shared<EventBusMock>();

    domain::TournamentFormat fmt(1, 1, domain::TournamentType::ROUND_ROBIN); // máx 1
    auto tour = std::make_shared<domain::Tournament>("T");
    tour->Id() = "t1";
    tour->Format() = fmt;

    auto group = std::make_shared<domain::Group>();
    group->Id() = "g1";
    group->TournamentId() = "t1";

    EXPECT_CALL(*tRepo, ReadById("t1")).WillOnce(Return(tour));
    EXPECT_CALL(*gRepo, GetGroup("t1", "g1", ::testing::_))
        .WillOnce(DoAll(
            ::testing::Invoke([&](std::string_view, std::string_view, std::shared_ptr<domain::Group>& out){
                out = group;
            }),
            Return(std::optional<std::string>{})
        ));
    EXPECT_CALL(*gRepo, CountTeamsInGroup("g1")).WillOnce(Return(1)); // ya lleno

    GroupDelegate delegate(gRepo, tRepo, teamRepo, bus);

    std::vector<domain::Team> add{ domain::Team{"team-2","Team 2"} };
    auto err = delegate.UpdateTeams("t1", "g1", add);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "group_full");
}

TEST(GroupDelegateEventsAndLimitsTest, AddTeam_TeamNotFound) {
    auto gRepo = std::make_shared<GroupRepoMock>();
    auto tRepo = std::make_shared<TournamentRepoMock>();
    auto teamRepo = std::make_shared<TeamRepoMock>();
    auto bus = std::make_shared<EventBusMock>();

    domain::TournamentFormat fmt(1, 3, domain::TournamentType::ROUND_ROBIN);
    auto tour = std::make_shared<domain::Tournament>("T");
    tour->Id() = "t1";
    tour->Format() = fmt;

    auto group = std::make_shared<domain::Group>();
    group->Id() = "g1";
    group->TournamentId() = "t1";

    EXPECT_CALL(*tRepo, ReadById("t1")).WillOnce(Return(tour));
    EXPECT_CALL(*gRepo, GetGroup("t1", "g1", ::testing::_))
        .WillOnce(DoAll(
            ::testing::Invoke([&](std::string_view, std::string_view, std::shared_ptr<domain::Group>& out){
                out = group;
            }),
            Return(std::optional<std::string>{})
        ));
    EXPECT_CALL(*gRepo, CountTeamsInGroup("g1")).WillOnce(Return(0));
    EXPECT_CALL(*teamRepo, ReadById("nope")).WillOnce(Return(nullptr));

    GroupDelegate delegate(gRepo, tRepo, teamRepo, bus);

    std::vector<domain::Team> add{ domain::Team{"nope","Unknown"} };
    auto err = delegate.UpdateTeams("t1", "g1", add);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, "team_not_found");
}
