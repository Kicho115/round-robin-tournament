#ifndef TOURNAMENTS_GROUPREPOSITORY_HPP
#define TOURNAMENTS_GROUPREPOSITORY_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

class GroupRepository : public IGroupRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    explicit GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);

    // IRepository
    std::shared_ptr<domain::Group> ReadById(std::string id) override;
    std::string Create(const domain::Group& entity) override;
    std::string Update(const domain::Group& entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Group>> ReadAll() override;

    // IGroupRepository - wrappers usados por GroupDelegate
    std::optional<std::string>
    GetGroups(std::string_view tournamentId,
              std::vector<std::shared_ptr<domain::Group>>& outGroups) override;

    std::optional<std::string>
    GetGroup(std::string_view tournamentId,
             std::string_view groupId,
             std::shared_ptr<domain::Group>& outGroup) override;

    // IGroupRepository - helpers también expuestos
    std::vector<std::shared_ptr<domain::Group>>
    FindByTournamentId(std::string_view tournamentId) override;

    std::shared_ptr<domain::Group>
    FindByTournamentIdAndGroupId(std::string_view tournamentId,
                                 std::string_view groupId) override;

    std::shared_ptr<domain::Group>
    FindByTournamentIdAndTeamId(std::string_view tournamentId,
                                std::string_view teamId) override;

    void
    UpdateGroupAddTeam(std::string_view groupId,
                       const std::shared_ptr<domain::Team>& team) override;

    bool
    ExistsGroupForTournament(std::string_view tournamentId) override;

    int
    GroupsCountForTournament(std::string_view tournamentId) override;

    int
    CountTeamsInGroup(std::string_view groupId) override;

    std::vector<domain::Team>
    GetTeamsOfGroup(std::string_view groupId) override;
};

#endif // TOURNAMENTS_GROUPREPOSITORY_HPP
