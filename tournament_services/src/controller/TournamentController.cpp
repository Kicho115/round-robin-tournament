//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <string>
#include <utility>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

// Nota: NO usamos 'using namespace crow::literals;' en este TU.

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate)
    : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    const std::string id = tournamentDelegate->CreateTournament(tournament);
    crow::response response;
    response.code = crow::CREATED;
    response.add_header("location", id);
    return response;
}

crow::response TournamentController::ReadAll() const {
    nlohmann::json body = tournamentDelegate->ReadAll();
    crow::response response;
    response.code = crow::OK;
    response.body = body.dump();
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

// Importante: si las rutas se registran con literales ("GET"_method), hazlo en un TU
// distinto que sí use crow::literals, o en tu router central. Aquí evitamos esa dependencia.
