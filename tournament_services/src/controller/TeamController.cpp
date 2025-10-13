// TeamController.cpp
// === Includes necesarios en este TU ===
#include "controller/TeamController.hpp"   // declara TeamController y teamDelegate
#include <crow.h>                          // crow::request, crow::response, status
#include <nlohmann/json.hpp>
#include <pqxx/except>                     // pqxx::unique_violation
#include <regex>
#include <string>
#include <string_view>
#include "domain/Utilities.hpp"            // <-- para ID_VALUE

// Nota: NO usamos 'using namespace crow::literals;' porque tu build no expone 'crow::literals'

crow::response TeamController::SaveTeam(const crow::request& request) const {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    const auto body = nlohmann::json::parse(request.body);
    domain::Team team = body;

    try {
        // Evita string_view colgante: SaveTeam devuelve std::string
        std::string createdId = teamDelegate->SaveTeam(team);
        if (createdId.empty()) {
            return crow::response{crow::CONFLICT, "duplicate team"};
        }

        crow::response res;
        res.code = crow::CREATED;
        res.add_header("location", createdId);
        return res;

    } catch (const pqxx::unique_violation&) {
        return crow::response{crow::CONFLICT, "duplicate team"};
    } catch (...) {
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

    auto body = nlohmann::json::parse(request.body);
    domain::Team team = body;
    team.Id = teamId;

    // C++17: UpdateTeam devuelve std::optional<std::string>
    // nullopt = OK; valor = mensaje de error
    auto err = teamDelegate->UpdateTeam(team);
    if (err.has_value()) {
        if (*err == "team_not_found") {
            return crow::response{crow::NOT_FOUND, "team not found"};
        }
        // Algunas versiones de Crow no tienen UNPROCESSABLE_ENTITY -> usar 422 directamente
        return crow::response{422, *err};
    }
    return crow::response{crow::NO_CONTENT};
}

crow::response TeamController::getTeam(const std::string& teamId) const {
    // Implementación mínima para pruebas
    return crow::response(200);
}

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

// === Registro de rutas ===
// Importante: si ya registras rutas en el .hpp o en un router central, NO las dupliques aquí.
// Además, evitar los literales "GET"_method / "POST"_method para no depender de crow::literals.
// Si necesitas registrar aquí sin literales, hazlo en tu archivo de rutas central.
