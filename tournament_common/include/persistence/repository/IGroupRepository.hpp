#ifndef COMMON_IGROUPREPOSITORY_HPP
#define COMMON_IGROUPREPOSITORY_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "IRepository.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

class IGroupRepository : public IRepository<domain::Group, std::string> {
public:
    ~IGroupRepository() override = default;

    // Usados por GroupDelegate
    virtual std::optional<std::string>
    GetGroups(std::string_view tournamentId,
              std::vector<std::shared_ptr<domain::Group>>& outGroups) = 0;

    virtual std::optional<std::string>
    GetGroup(std::string_view tournamentId,
             std::string_view groupId,
             std::shared_ptr<domain::Group>& outGroup) = 0;

    // Helpers también expuestos (los usan tests / consumer)
    virtual std::vector<std::shared_ptr<domain::Group>>
    FindByTournamentId(std::string_view tournamentId) = 0;

    virtual std::shared_ptr<domain::Group>
    FindByTournamentIdAndGroupId(std::string_view tournamentId,
                                 std::string_view groupId) = 0;

    virtual std::shared_ptr<domain::Group>
    FindByTournamentIdAndTeamId(std::string_view tournamentId,
                                std::string_view teamId) = 0;

    virtual void
    UpdateGroupAddTeam(std::string_view groupId,
                       const std::shared_ptr<domain::Team>& team) = 0;

    virtual bool
    ExistsGroupForTournament(std::string_view tournamentId) = 0;

    virtual int
    GroupsCountForTournament(std::string_view tournamentId) = 0;

    virtual int
    CountTeamsInGroup(std::string_view groupId) = 0;

    virtual std::vector<domain::Team>
    GetTeamsOfGroup(std::string_view groupId) = 0;
};

#endif // COMMON_IGROUPREPOSITORY_HPP
