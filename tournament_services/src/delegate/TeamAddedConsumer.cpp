#include "delegate/TeamAddedConsumer.hpp"
#include <regex>

namespace { const std::regex ID_VALUE{R"(^[A-Za-z0-9-]+$)"}; }

void TeamAddedConsumer::Handle(const nlohmann::json& eventJson) {
    HandleTeamAdded(TeamAddedEvent::FromJson(eventJson));
}

void TeamAddedConsumer::HandleTeamAdded(const std::string& tournamentId, const std::string& groupId) {
    TeamAddedEvent e;
    e.tournamentId = tournamentId;
    e.groupId = groupId;
    HandleTeamAdded(e);
}

void TeamAddedConsumer::HandleTeamAdded(const TeamAddedEvent& e) {
    if (e.tournamentId.empty() || e.groupId.empty()) return;
    if (!std::regex_match(e.tournamentId, ID_VALUE) ||
        !std::regex_match(e.groupId, ID_VALUE)) return;

    const int currentCount = groupRepo->CountTeamsInGroup(e.groupId);

    if (currentCount >= maxTeamsPerGroup) {
        nlohmann::json j = {
            {"tournamentId", e.tournamentId},
            {"groupId", e.groupId},
            {"format", "ROUND_ROBIN"}
        };
        publisher->Publish(topic, j.dump());
    }
}
