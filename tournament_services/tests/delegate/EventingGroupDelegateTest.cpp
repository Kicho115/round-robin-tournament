#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "delegate/EventingGroupDelegate.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "messaging/EventBus.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::_;
using ::testing::Return;

// ===== Mocks =====
class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD(std::optional<std::string>, CreateGroup,
        (std::string_view, const domain::Group&, std::string&), (override));
    MOCK_METHOD(std::optional<std::string>, GetGroups,
        (std::string_view, std::vector<std::shared_ptr<domain::Group>>&), (override));
    MOCK_METHOD(std::optional<std::string>, GetGroup,
        (std::string_view, std::string_view, std::shared_ptr<domain::Group>&), (override));
    MOCK_METHOD(std::optional<std::string>, UpdateGroup,
        (std::string_view, const domain::Group&), (override));
    MOCK_METHOD(std::optional<std::string>, RemoveGroup,
        (std::string_view, std::string_view), (override));
    MOCK_METHOD(std::optional<std::string>, UpdateTeams,
        (std::string_view, std::string_view, const std::vector<domain::Team>&), (override));
};

class EventBusMock : public IEventBus {
public:
    MOCK_METHOD(void, Publish, (std::string_view topic, const nlohmann::json& payload), (override));
};

// ===== Tests =====

TEST(EventingGroupDelegateTest, PublishesOneEventPerTeamOnSuccess) {
    auto inner = std::make_shared<GroupDelegateMock>();
    auto bus   = std::make_shared<EventBusMock>();
    EventingGroupDelegate decorator(inner, bus);

    const std::string tid = "t1";
    const std::string gid = "g1";
    std::vector<domain::Team> teams = {
        {"A","Team A"},
        {"B","Team B"}
    };

    // El inner retorna éxito (nullopt)
    EXPECT_CALL(*inner, UpdateTeams(tid, gid, _))
        .WillOnce(Return(std::nullopt));

    // Se publican 2 eventos (uno por cada team)
    EXPECT_CALL(*bus, Publish("team_added_to_group", _))
        .Times(2);

    auto res = decorator.UpdateTeams(tid, gid, teams);
    EXPECT_FALSE(res.has_value());
}

TEST(EventingGroupDelegateTest, DoesNotPublishOnError) {
    auto inner = std::make_shared<GroupDelegateMock>();
    auto bus   = std::make_shared<EventBusMock>();
    EventingGroupDelegate decorator(inner, bus);

    const std::string tid = "t1";
    const std::string gid = "g1";
    std::vector<domain::Team> teams = {
        {"X","Team X"}
    };

    // Error simulado (e.g., team_not_found o group_full)
    EXPECT_CALL(*inner, UpdateTeams(tid, gid, _))
        .WillOnce(Return(std::make_optional<std::string>("team_not_found")));

    // No se publica nada en error
    EXPECT_CALL(*bus, Publish(_, _)).Times(0);

    auto res = decorator.UpdateTeams(tid, gid, teams);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(*res, "team_not_found");
}
