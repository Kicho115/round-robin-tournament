#include <regex>
#include <nlohmann/json.hpp>
#include <crow.h>

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using nlohmann::json;

// Patrón simple para IDs (coincidir con el de tus otros controladores)
namespace {
    const std::regex ID_VALUE{R"(^[A-Za-z0-9-]+$)"};
}

// === ctor/dtor como en el .hpp ===
GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
    : groupDelegate(delegate) {}

GroupController::~GroupController() = default;

// GET /tournaments/{tid}/groups
crow::response GroupController::GetGroups(const std::string& tournamentId) {
    if (!std::regex_match(tournamentId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    std::vector<std::shared_ptr<domain::Group>> groups;
    if (auto err = groupDelegate->GetGroups(tournamentId, groups)) {
        // Mapeo simple de errores
        if (*err == "tournament_not_found") return crow::response{crow::NOT_FOUND, *err};
        return crow::response{422, *err}; // 422 en lugar de crow::UNPROCESSABLE (no existe en algunas versiones)
    }

    json body = json::array();
    for (auto& g : groups) body.push_back(json(*g));
    crow::response res{crow::OK, body.dump()};
    res.add_header("content-type", "application/json");
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
    res.add_header("content-type", "application/json");
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
    domain::Group group = body; // requiere from_json(domain::Group)
    std::string newGroupId;

    if (auto err = groupDelegate->CreateGroup(tournamentId, group, newGroupId)) {
        if (*err == "duplicate_group_name" || *err == "group_limit_reached") {
            return crow::response{crow::CONFLICT, *err}; // 409
        }
        if (*err == "tournament_not_found") {
            return crow::response{crow::NOT_FOUND, *err}; // 404
        }
        return crow::response{422, *err}; // genérico
    }

    crow::response res{crow::CREATED};
    // Según tests, a veces esperan solo el id; si prefieres ruta completa, cambia por "/tournaments/"+tournamentId+"/groups/"+newGroupId
    res.add_header("location", newGroupId.c_str());
    return res;
}

// PATCH /tournaments/{tid}/groups  (según tu .hpp sin ids en ruta — aquí lo dejo como no implementado)
crow::response GroupController::UpdateGroup(const crow::request& /*request*/) {
    return crow::response{crow::NOT_IMPLEMENTED};
}

// PUT /tournaments/{tid}/groups/{gid}/teams
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
            return crow::response{422, *err}; // reglas de negocio
        }
        return crow::response{422, *err};
    }

    return crow::response{crow::NO_CONTENT};
}

// Nota: No registramos rutas aquí si ya las defines en el .hpp con REGISTER_ROUTE.
