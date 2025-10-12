#include "delegate/GroupDelegate.hpp"
#include <nlohmann/json.hpp>
#include <pqxx/except>
#include <memory>
#include <utility>
#include <type_traits>
#include <optional>
#include <string>

using nlohmann::json;

// Helper: normaliza std::optional<T> a std::shared_ptr<domain::Group>
// T puede ser domain::Group o std::shared_ptr<domain::Group>
template <typename Opt>
static std::shared_ptr<domain::Group> to_shared_ptr(const Opt& opt) {
    using ValT = typename std::decay_t<Opt>::value_type;
    if constexpr (std::is_same_v<ValT, std::shared_ptr<domain::Group>>) {
        return opt.value(); // ya es shared_ptr
    } else {
        return std::make_shared<domain::Group>(opt.value()); // viene por valor
    }
}

std::optional<std::string>
GroupDelegate::CreateGroup(std::string_view tournamentId, const domain::Group& group, std::string& outGroupId) {
    if (!tournamentRepository->ReadById(std::string(tournamentId))) {
        return std::make_optional<std::string>("tournament_not_found");
    }
    // Torneo solo permite 1 grupo
    if (groupRepository->ExistsGroupForTournament(std::string(tournamentId))) {
        return std::make_optional<std::string>("group_limit_reached");
    }
    try {
        outGroupId = groupRepository->Create(group);
        json ev{
            {"type","group.created"},
            {"tournamentId", std::string(tournamentId)},
            {"groupId", outGroupId},
            {"name", group.Name()}
        };
        if (producer) producer->SendMessage(ev.dump(), "group.created");
        return std::nullopt;
    } catch (const pqxx::unique_violation&) {
        return std::make_optional<std::string>("duplicate_group_name");
    } catch (const std::exception& e) {
        return std::make_optional<std::string>(e.what());
    }
}

std::optional<std::string>
GroupDelegate::GetGroups(std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups) {
    try {
        outGroups = groupRepository->FindByTournamentId(std::string(tournamentId));
        return std::nullopt;
    } catch (const std::exception& e) {
        return std::make_optional<std::string>(e.what());
    }
}

std::optional<std::string>
GroupDelegate::GetGroup(std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup) {
    (void)tournamentId; // usar si el repo lo requiere
    auto found = groupRepository->FindByTournamentIdAndGroupId(std::string(tournamentId), std::string(groupId));
    if (!found) {
        return std::make_optional<std::string>("group_not_found");
    }
    outGroup = to_shared_ptr(found);
    return std::nullopt;
}

std::optional<std::string>
GroupDelegate::UpdateGroup(std::string_view /*tournamentId*/, const domain::Group& /*group*/) {
    return std::make_optional<std::string>("not_implemented");
}

std::optional<std::string>
GroupDelegate::RemoveGroup(std::string_view /*tournamentId*/, std::string_view /*groupId*/) {
    return std::make_optional<std::string>("not_implemented");
}

std::optional<std::string>
GroupDelegate::UpdateTeams(std::string_view tournamentId, std::string_view groupId, const std::vector<domain::Team>& teams) {
    if (!tournamentRepository->ReadById(std::string(tournamentId))) {
        return std::make_optional<std::string>("tournament_not_found");
    }
    if (!groupRepository->FindByTournamentIdAndGroupId(std::string(tournamentId), std::string(groupId))) {
        return std::make_optional<std::string>("group_not_found");
    }

    try {
        // groupRepository->ReplaceTeams(std::string(groupId), teams); // si existe
        json ev{
            {"type","group.teams_updated"},
            {"tournamentId", std::string(tournamentId)},
            {"groupId", std::string(groupId)},
            {"teams", teams}
        };
        if (producer) producer->SendMessage(ev.dump(), "group.teams_updated");
        return std::nullopt;
    } catch (const std::exception& e) {
        return std::make_optional<std::string>(e.what());
    }
}
