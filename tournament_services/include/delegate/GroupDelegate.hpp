#pragma once
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "delegate/IGroupDelegate.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/ITournamentRepository.hpp"
#include "persistence/repository/ITeamRepository.hpp"
#include "messaging/IEventBus.hpp"

class GroupDelegate : public IGroupDelegate {
    std::shared_ptr<IGroupRepository> groupRepo;
    std::shared_ptr<ITournamentRepository> tournamentRepo; // puede ser nullptr
    std::shared_ptr<ITeamRepository> teamRepo;             // puede ser nullptr
    std::shared_ptr<IEventBus> eventBus;                   // puede ser nullptr

public:
    // Constructor existente (no lo quites):
    explicit GroupDelegate(std::shared_ptr<IGroupRepository> groupRepo);

    // inyección completa (para límites y eventos)
    GroupDelegate(std::shared_ptr<IGroupRepository> groupRepo,
                  std::shared_ptr<ITournamentRepository> tournamentRepo,
                  std::shared_ptr<ITeamRepository> teamRepo,
                  std::shared_ptr<IEventBus> eventBus);

    // Métodos IGroupDelegate
    std::optional<std::string>
    CreateGroup(std::string_view tournamentId, const domain::Group& group, std::string& outGroupId) override;

    std::optional<std::string>
    GetGroups(std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups) override;

    std::optional<std::string>
    GetGroup(std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup) override;

    std::optional<std::string>
    UpdateGroup(std::string_view tournamentId, const domain::Group& group) override;

    std::optional<std::string>
    RemoveGroup(std::string_view tournamentId, std::string_view groupId) override;

    std::optional<std::string>
    UpdateTeams(std::string_view tournamentId, std::string_view groupId, const std::vector<domain::Team>& teams) override;
};
