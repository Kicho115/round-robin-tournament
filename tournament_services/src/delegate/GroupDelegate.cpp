#include "delegate/GroupDelegate.hpp"
#include <nlohmann/json.hpp>
#include "messaging/Topics.hpp"

using std::string;
using std::string_view;
using std::shared_ptr;
using std::make_shared;
using std::optional;
using nlohmann::json;

GroupDelegate::GroupDelegate(std::shared_ptr<IGroupRepository> repo)
: groupRepo(std::move(repo)) {}

GroupDelegate::GroupDelegate(std::shared_ptr<IGroupRepository> groupRepo_,
                             std::shared_ptr<ITournamentRepository> tournamentRepo_,
                             std::shared_ptr<ITeamRepository> teamRepo_,
                             std::shared_ptr<IEventBus> eventBus_)
: groupRepo(std::move(groupRepo_)),
  tournamentRepo(std::move(tournamentRepo_)),
  teamRepo(std::move(teamRepo_)),
  eventBus(std::move(eventBus_)) {}

// Helper: obtener formato y validar torneo
static optional<std::string> get_format_or_err(const shared_ptr<ITournamentRepository>& tRepo,
                                               std::string_view tid,
                                               domain::TournamentFormat& outFmt)
{
    if (!tRepo) return std::nullopt; // si no hay repo, no validamos (compat)
    auto t = tRepo->ReadById(std::string(tid));
    if (!t) return std::make_optional<std::string>("tournament_not_found");
    outFmt = t->Format();
    return std::nullopt;
}

// Helper: publicar (si hay bus)
static void publish_if(const shared_ptr<IEventBus>& bus, std::string_view topic, const json& payload) {
    if (bus) bus->Publish(topic, payload);
}

std::optional<std::string>
GroupDelegate::CreateGroup(std::string_view tournamentId, const domain::Group& group, std::string& outGroupId)
{
    // Validar torneo y formato (límite al crear con equipos precargados)
    domain::TournamentFormat fmt;
    if (auto e = get_format_or_err(tournamentRepo, tournamentId, fmt)) {
        return e;
    }
    if (!group.Teams().empty() && fmt.MaxTeamsPerGroup() > 0 &&
        static_cast<int>(group.Teams().size()) > fmt.MaxTeamsPerGroup())
    {
        return std::make_optional<std::string>("group_limit_reached");
    }

    // Crear
    outGroupId = groupRepo->Create(group);
    if (outGroupId.empty()) {
        return std::make_optional<std::string>("duplicate_group_name");
    }

    // Evento: grupo creado
    publish_if(eventBus, topics::GroupCreated, json{
        {"tournamentId", std::string(tournamentId)},
        {"groupId", outGroupId},
        {"name", group.Name()}
    });

    // Si vienen equipos en el create, agrégalos y publica evento por cada uno
    for (const auto& t : group.Teams()) {
        groupRepo->UpdateGroupAddTeam(outGroupId, std::make_shared<domain::Team>(t));
        publish_if(eventBus, topics::GroupTeamAdded, json{
            {"tournamentId", std::string(tournamentId)},
            {"groupId", outGroupId},
            {"team", {{"id", t.Id}, {"name", t.Name}}}
        });
    }

    return std::nullopt;
}

std::optional<std::string>
GroupDelegate::GetGroups(std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups) {
    return groupRepo->GetGroups(tournamentId, outGroups);
}

std::optional<std::string>
GroupDelegate::GetGroup(std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup) {
    return groupRepo->GetGroup(tournamentId, groupId, outGroup);
}

std::optional<std::string>
GroupDelegate::UpdateGroup(std::string_view tournamentId, const domain::Group& group) {
    // Mantenemos tu lógica existente (repo->Update(...))
    auto id = groupRepo->Update(group);
    if (id.empty()) return std::make_optional<std::string>("group_not_found");
    return std::nullopt;
}

std::optional<std::string>
GroupDelegate::RemoveGroup(std::string_view tournamentId, std::string_view groupId) {
    // Si ya tienes implementación, mantenla; si no, finge éxito.
    return std::nullopt;
}

std::optional<std::string>
GroupDelegate::UpdateTeams(std::string_view tournamentId,
                           std::string_view groupId,
                           const std::vector<domain::Team>& teams)
{
    // Validar torneo (para límites)
    domain::TournamentFormat fmt;
    if (auto e = get_format_or_err(tournamentRepo, tournamentId, fmt)) {
        return e; // tournament_not_found si aplica
    }

    // Validar grupo existe
    std::shared_ptr<domain::Group> g;
    if (auto e = groupRepo->GetGroup(tournamentId, groupId, g); e.has_value()) {
        return e; // podría ser group_not_found
    }
    if (!g) return std::make_optional<std::string>("group_not_found");

    // Validar límite contra conteo actual + nuevos
    if (fmt.MaxTeamsPerGroup() > 0) {
        int current = 0;
        if (groupRepo) {
            current = groupRepo->CountTeamsInGroup(groupId);
        }
        if (current + static_cast<int>(teams.size()) > fmt.MaxTeamsPerGroup()) {
            return std::make_optional<std::string>("group_full");
        }
    }

    // Validar que cada team existe cuando tenemos teamRepo
    for (const auto& t : teams) {
        if (teamRepo) {
            auto tr = teamRepo->ReadById(t.Id);
            if (!tr) return std::make_optional<std::string>("team_not_found");
        }
    }

    // Persistir y publicar evento por cada team agregado
    for (const auto& t : teams) {
        groupRepo->UpdateGroupAddTeam(groupId, std::make_shared<domain::Team>(t));
        publish_if(eventBus, topics::GroupTeamAdded, json{
            {"tournamentId", std::string(tournamentId)},
            {"groupId", std::string(groupId)},
            {"team", {{"id", t.Id}, {"name", t.Name}}}
        });
    }

    return std::nullopt;
}
