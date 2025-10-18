// TeamController.cpp

#include "controller/TeamController.hpp"

#include <optional>
#include <pqxx/except>   // pqxx::unique_violation
#include <string>
#include <string_view>

#include "domain/Team.hpp"

// --------------------------------------------------------

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

// GET /teams/<id>
crow::response TeamController::getTeam(const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST};
    }

    auto t = teamDelegate->GetTeam(teamId);
    if (!t) {
        return crow::response{crow::NOT_FOUND};
    }

    nlohmann::json body{
        {"id",   t->Id},
        {"name", t->Name}
    };

    crow::response res;
    res.code = crow::OK;
    res.add_header("content-type", "application/json");
    res.body = body.dump();
    return res;
}

// GET /teams  (firma usada por los tests)
crow::response TeamController::GetTeams(const crow::request& /*request*/) const {
    return getAllTeams();
}

// Helper GET /teams (sin request)
crow::response TeamController::getAllTeams() const {
    const auto teams = teamDelegate->GetAllTeams();

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& t : teams) {
        if (!t) continue;
        nlohmann::json item{
            {"id",   t->Id},
            {"name", t->Name}
        };
        arr.push_back(std::move(item));
    }

    crow::response res;
    res.code = crow::OK;
    res.add_header("content-type", "application/json");
    res.body = arr.dump();
    return res;
}

// POST /teams
crow::response TeamController::SaveTeam(const crow::request& request) const {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    const auto body = nlohmann::json::parse(request.body);

    domain::Team team;
    // 'id' es opcional: normalmente lo genera la BD
    if (body.contains("id") && body["id"].is_string()) {
        team.Id = body["id"].get<std::string>();
    }
    if (body.contains("name") && body["name"].is_string()) {
        team.Name = body["name"].get<std::string>();
    }

    try {
        std::string createdId = teamDelegate->SaveTeam(team);

        // Contrato: "" => duplicado (o cualquier error de conflicto)
        if (createdId.empty()) {
            return crow::response{crow::CONFLICT, "duplicate team"};
        }

        crow::response res;
        res.code = crow::CREATED;
        res.add_header("location", createdId);
        return res;
    }
    catch (const pqxx::unique_violation&) {
        return crow::response{crow::CONFLICT, "duplicate team"};
    }
    catch (...) {
        // opcional: puedes mapear otros errores
        return crow::response{crow::INTERNAL_SERVER_ERROR};
    }
}

// PATCH /teams/<id>
crow::response TeamController::UpdateTeam(const crow::request& request,
                                          const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    const auto body = nlohmann::json::parse(request.body);

    domain::Team team;
    team.Id = teamId;
    if (body.contains("name") && body["name"].is_string()) {
        team.Name = body["name"].get<std::string>();
    }

    // Contrato: nullopt => OK; string => mensaje de error
    auto err = teamDelegate->UpdateTeam(team);
    if (err.has_value()) {
        if (*err == "team_not_found") {
            return crow::response{crow::NOT_FOUND, "team not found"};
        }
        return crow::response{422, *err};
    }

    return crow::response{crow::NO_CONTENT};
}
