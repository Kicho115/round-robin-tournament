#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "delegate/GroupDelegate.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/QueueMessageProducer.hpp"

using ::testing::Return;
using ::testing::_;
using nlohmann::json;

// Mocks minimalistas (interfaz real puede variar levemente)
class GroupRepoMock : public GroupRepository {
public:
    using GroupRepository::GroupRepository;
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string&), (override));
    MOCK_METHOD(std::optional<std::shared_ptr<domain::Group>>, FindByTournamentIdAndGroupId, (const std::string&, const std::string&), (override));
    MOCK_METHOD(std::optional<std::shared_ptr<domain::Group>>, FindByTournamentIdAndTeamId, (const std::string&, const std::string&), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group&), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string&, const std::shared_ptr<domain::Team>&), (override));
    MOCK_METHOD(int, CountTeamsInGroup, (const std::string_view&), (override));
    MOCK_METHOD(int, GroupCapacityForTournament, (const std::string_view&), (override));
};

class TeamRepoMock : public TeamRepository {
public:
    using TeamRepository::TeamRepository;
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (const std::string&), (override));
};

class TournamentRepoMock : public TournamentRepository {
public:
    using TournamentRepository::TournamentRepository;
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string&), (override));
};

class BusMock : public QueueMessageProducer {
public:
    using QueueMessageProducer::QueueMessageProducer;
    MOCK_METHOD(void, SendMessage, (const std::string&, const std::string&), (override));
};

TEST(GroupDelegate, AddTeam_Ok_PublishesEvent) {
    auto groups = std::make_shared<GroupRepoMock>(nullptr,nullptr);
    auto teams  = std::make_shared<TeamRepoMock>(nullptr,nullptr);
    auto tours  = std::make_shared<TournamentRepoMock>(nullptr,nullptr);
    auto bus    = std::make_shared<BusMock>(nullptr);

    EXPECT_CALL(*tours, ReadById(_)).WillOnce(Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groups, FindByTournamentIdAndGroupId(_, _)).WillOnce(Return(std::make_shared<domain::Group>()));
    EXPECT_CALL(*teams,  ReadById(_)).WillOnce(Return(std::make_shared<domain::Team>()));
    EXPECT_CALL(*groups, FindByTournamentIdAndTeamId(_, _)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*groups, GroupCapacityForTournament(_)).WillOnce(Return(4));
    EXPECT_CALL(*groups, CountTeamsInGroup(_)).WillOnce(Return(3));
    EXPECT_CALL(*groups, UpdateGroupAddTeam(_, _)).Times(1);
    EXPECT_CALL(*bus, SendMessage(::testing::HasSubstr("\"type\":\"group.team_added\""), "group.team_added")).Times(1);

    GroupDelegate sut(groups, teams, tours, bus);
    auto r = sut.AddTeamToGroup("tid","gid","team1","A");
    ASSERT_TRUE(r.has_value());
}

TEST(GroupDelegate, AddTeam_GroupFull) {
    auto groups = std::make_shared<GroupRepoMock>(nullptr,nullptr);
    auto teams  = std::make_shared<TeamRepoMock>(nullptr,nullptr);
    auto tours  = std::make_shared<TournamentRepoMock>(nullptr,nullptr);
    auto bus    = std::make_shared<BusMock>(nullptr);

    EXPECT_CALL(*tours, ReadById(_)).WillOnce(Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groups, FindByTournamentIdAndGroupId(_, _)).WillOnce(Return(std::make_shared<domain::Group>()));
    EXPECT_CALL(*teams,  ReadById(_)).WillOnce(Return(std::make_shared<domain::Team>()));
    EXPECT_CALL(*groups, FindByTournamentIdAndTeamId(_, _)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*groups, GroupCapacityForTournament(_)).WillOnce(Return(4));
    EXPECT_CALL(*groups, CountTeamsInGroup(_)).WillOnce(Return(4)); // lleno

    GroupDelegate sut(groups, teams, tours, bus);
    auto r = sut.AddTeamToGroup("tid","gid","team1","A");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ("group_full", r.error());
}
