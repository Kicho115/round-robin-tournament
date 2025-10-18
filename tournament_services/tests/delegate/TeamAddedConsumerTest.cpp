#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>

#include "persistence/repository/IGroupRepository.hpp"
#include "delegate/TeamAddedConsumer.hpp"
// Si este test usa el publisher, incluye la interfaz real (no fwd-declare)
#include "delegate/IEventPublisher.hpp"

using ::testing::StrictMock;

class GroupRepositoryMock : public IGroupRepository {
public:
    // IRepository
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group&), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Group>>), ReadAll, (), (override));

    // Métodos usados por GroupDelegate
    MOCK_METHOD((std::optional<std::string>), GetGroups,
                (std::string_view, std::vector<std::shared_ptr<domain::Group>>&), (override));

    MOCK_METHOD((std::optional<std::string>), GetGroup,
                (std::string_view, std::string_view, std::shared_ptr<domain::Group>&), (override));

    // Helpers expuestos y usados en consumer/tests
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Group>>), FindByTournamentId,
                (std::string_view), (override));

    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId,
                (std::string_view, std::string_view), (override));

    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId,
                (std::string_view, std::string_view), (override));

    MOCK_METHOD(void, UpdateGroupAddTeam,
                (std::string_view, const std::shared_ptr<domain::Team>&), (override));

    MOCK_METHOD(bool, ExistsGroupForTournament, (std::string_view), (override));
    MOCK_METHOD(int, GroupsCountForTournament, (std::string_view), (override));
    MOCK_METHOD(int, CountTeamsInGroup, (std::string_view), (override));
    MOCK_METHOD((std::vector<domain::Team>), GetTeamsOfGroup, (std::string_view), (override));
};
