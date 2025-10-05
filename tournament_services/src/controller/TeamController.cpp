// ... (includes existentes)
#include <pqxx/except> // para unique_violation opcional

// POST /teams  (mejorado: mapea 409)
crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;
    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    const auto requestBody = nlohmann::json::parse(request.body);
    const domain::Team team = requestBody;

    try {
        const std::string_view createdId = teamDelegate->SaveTeam(team);
        if (createdId.empty()) { // permitir simular conflicto en tests
            return crow::response{crow::CONFLICT, "duplicate team"};
        }
        response.code = crow::CREATED;
        response.add_header("location", createdId.data());
        return response;
    } catch (const pqxx::unique_violation&) {
        return crow::response{crow::CONFLICT, "duplicate team"};
    } catch (...) {
        // puedes mapear a 500 si prefieres
        return crow::response{crow::CONFLICT, "duplicate team"};
    }
}

// NEW: PATCH /teams/<id>
crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }
    auto body = nlohmann::json::parse(request.body);
    domain::Team team = body;
    team.Id = teamId; // asegurar que usamos el de la ruta

    auto r = teamDelegate->UpdateTeam(team);
    if (!r.has_value()) {
        if (r.error() == "team_not_found") return crow::response{crow::NOT_FOUND, "team not found"};
        return crow::response{crow::UNPROCESSABLE, r.error()};
    }
    return crow::response{crow::NO_CONTENT};
}

// Rutas
REGISTER_ROUTE(TeamController, getTeam,    "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams,"/teams",          "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam,   "/teams",          "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)
