#include <regex>
#include <nlohmann/json.hpp>
#include <crow.h>

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using nlohmann::json;

namespace {
    // Ajusta el patrón si tus IDs usan otro formato.
    const std::regex ID_VALUE{R"(^[A-Za-z0-9-]+$)"};
}

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
    : groupDelegate(delegate) {}

// GET /tournaments/{tid}/groups
crow::response GroupController::GetGroups(const std::string& tournamentId) {
    if (!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    std::vector<std::shared_ptr<domain::Group>> groups;
    if (auto err = groupDelegate->GetGroups(tournamentId, groups)) {
        if (*err == "tournament_not_found") return crow::response{crow::NOT_FOUND, *err};
        return crow::response{422, *err};
    }

    json body = json::array();
    for (auto& g : groups) body.push_back(json(*g));

    crow::response res{crow::OK, body.dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

// GET /tournaments/{tid}/groups/{gid}
crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) {
    if (!std::regex_match(tournamentId, ID_VALUE) || !std::regex_match(groupId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    std::shared_ptr<domain::Group> group;
    if (auto err = groupDelegate->GetGroup(tournamentId, groupId, group)) {
        if (*err == "tournament_not_found" || *err == "group_not_found") {
            return crow::response{crow::NOT_FOUND, *err};
        }
        return crow::response{422, *err};
    }

    json body = *group;
    crow::response res{crow::OK, body.dump()};
    res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return res;
}

// POST /tournaments/{tid}/groups
crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId) {
    if (!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    const auto body = json::parse(request.body);
    domain::Group group = body; // requiere adl_serializer<domain::Group>
    std::string newGroupId;

    if (auto err = groupDelegate->CreateGroup(tournamentId, group, newGroupId)) {
        if (*err == "duplicate_group_name" || *err == "group_limit_reached") {
            return crow::response{crow::CONFLICT, *err};
        }
        if (*err == "tournament_not_found") {
            return crow::response{crow::NOT_FOUND, *err};
        }
        return crow::response{422, *err};
    }

    crow::response res{crow::CREATED};
    res.add_header("location", newGroupId.c_str());
    return res;
}

// PATCH /tournaments/{tid}/groups/{gid}
crow::response GroupController::UpdateGroup(const crow::request& request,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    if (!std::regex_match(tournamentId, ID_VALUE) || !std::regex_match(groupId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    // Parseo defensivo: sólo extrae lo que necesitas.
    json body = json::parse(request.body);
    if (!body.is_object()) {
        return crow::response{crow::BAD_REQUEST};
    }

    domain::Group group{};
    group.Id() = groupId;
    group.TournamentId() = tournamentId;

    if (auto it = body.find("name"); it != body.end() && it->is_string()) {
        group.Name() = it->get<std::string>();
    }

    if (auto err = groupDelegate->UpdateGroup(tournamentId, group)) {
        if (*err == "group_not_found" || *err == "tournament_not_found") {
            return crow::response{crow::NOT_FOUND, *err};
        }
        return crow::response{422, *err};
    }

    return crow::response{crow::NO_CONTENT};
}

// PATCH /tournaments/{tid}/groups/{gid}/teams (reemplazo/actualización por lote)
crow::response GroupController::UpdateTeams(const crow::request& request,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    if (!std::regex_match(tournamentId, ID_VALUE) || !std::regex_match(groupId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    const std::vector<domain::Team> teams = json::parse(request.body);
    if (auto err = groupDelegate->UpdateTeams(tournamentId, groupId, teams)) {
        if (*err == "tournament_not_found" || *err == "group_not_found") {
            return crow::response{crow::NOT_FOUND, *err};
        }
        if (*err == "group_full" || *err == "team_already_in_group" || *err == "team_not_found") {
            return crow::response{422, *err};
        }
        return crow::response{422, *err};
    }

    return crow::response{crow::NO_CONTENT};
}

// POST /tournaments/{tid}/groups/{gid} (agregar un equipo individual)
crow::response GroupController::AddTeam(const crow::request& request,
                                        const std::string& tournamentId,
                                        const std::string& groupId) {
    if (!std::regex_match(tournamentId, ID_VALUE) || !std::regex_match(groupId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    const domain::Team team = json::parse(request.body);
    const std::vector<domain::Team> teams{team};

    if (auto err = groupDelegate->UpdateTeams(tournamentId, groupId, teams)) {
        if (*err == "tournament_not_found" || *err == "group_not_found") {
            return crow::response{crow::NOT_FOUND, *err};
        }
        if (*err == "group_full" || *err == "team_already_in_group" || *err == "team_not_found") {
            return crow::response{422, *err};
        }
        return crow::response{422, *err};
    }

    return crow::response{crow::CREATED};
}

// Rutas
REGISTER_ROUTE(GroupController, GetGroups,   "/tournaments/<string>/groups",                 "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup,    "/tournaments/<string>/groups/<string>",       "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups",                "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>",       "PATCH"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
REGISTER_ROUTE(GroupController, AddTeam,     "/tournaments/<string>/groups/<string>",       "POST"_method)
