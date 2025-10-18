#include "delegate/EventingGroupDelegate.hpp"
#include <nlohmann/json.hpp>

std::optional<std::string>
EventingGroupDelegate::UpdateTeams(std::string_view tournamentId,
                                   std::string_view groupId,
                                   const std::vector<domain::Team>& teams) {
    auto res = inner->UpdateTeams(tournamentId, groupId, teams);

    // Si inner retornó éxito (nullopt), publicamos un evento por cada equipo agregado.
    if (!res.has_value()) {
        for (const auto& t : teams) {
            nlohmann::json payload = {
                {"event",         "team_added_to_group"},
                {"tournamentId",  std::string(tournamentId)},
                {"groupId",       std::string(groupId)},
                {"teamId",        t.Id},
                {"teamName",      t.Name}
            };
            bus->Publish("team_added_to_group", payload);
        }
    }
    return res;
}
