//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include  "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
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

crow::response TournamentController::GetById(const crow::request &request, const std::string& id) const {
    std::cout << "GET /tournaments/{} - Buscando torneo con ID: {}" << request.body << std::endl;
    auto tournament = tournamentDelegate->ReadById(id);
    
    crow::response response;
    
    if (!tournament) {
        response.code = crow::NOT_FOUND;
        response.body = R"({"error": "Tournament not found"})";
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    
    nlohmann::json body = tournament;
    response.code = crow::OK;
    response.body = body.dump();
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    
    return response;
}


REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, GetById, "/tournaments/<string>", "GET"_method)