#pragma once
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "delegate/IGroupDelegate.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

// IMPORTANTE: usar solo la interfaz, no la implementación concreta de ActiveMQ.
// Ajusta la ruta según tu árbol: "cms/IQueueMessageProducer.hpp" o similar.
#include "cms/IQueueMessageProducer.hpp"

class GroupDelegate : public IGroupDelegate {
public:
    GroupDelegate(std::shared_ptr<GroupRepository> groups,
                  std::shared_ptr<TeamRepository> teams,
                  std::shared_ptr<TournamentRepository> tours,
                  std::shared_ptr<IQueueMessageProducer> bus);

    virtual ~GroupDelegate();

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

private:
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<TeamRepository> teamRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IQueueMessageProducer> producer; // interfaz -> no arrastra activemq-cpp
};
