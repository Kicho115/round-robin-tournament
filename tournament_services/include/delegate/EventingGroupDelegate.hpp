#ifndef SERVICES_EVENTING_GROUP_DELEGATE_HPP
#define SERVICES_EVENTING_GROUP_DELEGATE_HPP

#include <memory>
#include <string_view>
#include <vector>
#include <optional>

#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "messaging/EventBus.hpp"

class EventingGroupDelegate : public IGroupDelegate {
    std::shared_ptr<IGroupDelegate> inner;
    std::shared_ptr<IEventBus>      bus;

public:
    EventingGroupDelegate(std::shared_ptr<IGroupDelegate> innerDelegate,
                          std::shared_ptr<IEventBus> eventBus)
        : inner(std::move(innerDelegate)), bus(std::move(eventBus)) {}

    // === IGroupDelegate (en su mayoría, se reenvían al inner) ===
    std::optional<std::string>
    CreateGroup(std::string_view tournamentId, const domain::Group& group, std::string& outGroupId) override {
        return inner->CreateGroup(tournamentId, group, outGroupId);
    }

    std::optional<std::string>
    GetGroups(std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups) override {
        return inner->GetGroups(tournamentId, outGroups);
    }

    std::optional<std::string>
    GetGroup(std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup) override {
        return inner->GetGroup(tournamentId, groupId, outGroup);
    }

    std::optional<std::string>
    UpdateGroup(std::string_view tournamentId, const domain::Group& group) override {
        return inner->UpdateGroup(tournamentId, group);
    }

    std::optional<std::string>
    RemoveGroup(std::string_view tournamentId, std::string_view groupId) override {
        return inner->RemoveGroup(tournamentId, groupId);
    }

    // Aquí agregamos la publicación de eventos si el inner retorna éxito.
    std::optional<std::string>
    UpdateTeams(std::string_view tournamentId, std::string_view groupId, const std::vector<domain::Team>& teams) override;
};

#endif // SERVICES_EVENTING_GROUP_DELEGATE_HPP
