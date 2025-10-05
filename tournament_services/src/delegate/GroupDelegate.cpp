#include <nlohmann/json.hpp>
#include "delegate/GroupDelegate.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/QueueMessageProducer.hpp"

using nlohmann::json;

GroupDelegate::GroupDelegate(std::shared_ptr<GroupRepository> groups,
                             std::shared_ptr<TeamRepository> teams,
                             std::shared_ptr<TournamentRepository> tours,
                             std::shared_ptr<QueueMessageProducer> producer)
: groupRepository(std::move(groups)),
  teamRepository(std::move(teams)),
  tournamentRepository(std::move(tours)),
  producer(std::move(producer)) {}

json GroupDelegate::ReadByTournament(const std::string& tournamentId) const {
    json out = json::array();
    auto groups = groupRepository->FindByTournamentId(tournamentId);
    for (auto& g : groups) out.push_back(json(*g));
    return out;
}

std::expected<std::string,std::string>
GroupDelegate::CreateGroup(const std::string& tournamentId,
                           const std::string& name,
                           const nlohmann::json& teams) const {
    if (!tournamentRepository->ReadById(tournamentId)) return std::unexpected("tournament_not_found");

    domain::Group g;
    g.TournamentId() = tournamentId;
    g.Name() = name;
    if (teams.is_array()) {
        for (auto& t : teams) g.Teams().push_back(std::make_shared<domain::Team>(domain::Team{t}));
    }
    try {
        auto id = groupRepository->Create(g);
        // Evento opcional
        json ev{{"type","group.created"},{"tournamentId",tournamentId},{"groupId",id},{"name",name}};
        producer->SendMessage(ev.dump(), "group.created"); // mismo patrón que TournamentDelegate
        return id;
    } catch (const std::exception& e) {
        return std::unexpected(e.what());
    }
}

std::expected<void,std::string>
GroupDelegate::AddTeamToGroup(const std::string& tournamentId, const std::string& groupId,
                              const std::string& teamId, const std::string& teamName) const {
    // 1) torneo y grupo
    if (!tournamentRepository->ReadById(tournamentId)) return std::unexpected("tournament_not_found");
    if (!groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId)) return std::unexpected("group_not_found");

    // 2) equipo existe
    if (!teamRepository->ReadById(teamId)) return std::unexpected("team_not_found");

    // 3) evitar duplicado en otro grupo del mismo torneo
    if (groupRepository->FindByTournamentIdAndTeamId(tournamentId, teamId)) return std::unexpected("team_already_in_group");

    // 4) aforo por torneo (0 = sin límite)
    const int cap = groupRepository->GroupCapacityForTournament(tournamentId);
    if (cap > 0) {
        const int cur = groupRepository->CountTeamsInGroup(groupId);
        if (cur >= cap) return std::unexpected("group_full");
    }

    // 5) persistir (usa prepared update_group_add_team de tu repo)
    auto t = std::make_shared<domain::Team>();
    t->Id() = teamId; t->Name() = teamName;
    groupRepository->UpdateGroupAddTeam(groupId, t);

    // 6) publicar evento para que el CONSUMIDOR RR programe los partidos al llenarse
    json ev{
        {"type","group.team_added"},
        {"tournamentId", tournamentId},
        {"groupId", groupId},
        {"team", {{"id",teamId},{"name",teamName}}}
    };
    producer->SendMessage(ev.dump(), "group.team_added"); // mismo patrón que torneos
    return {};
}
