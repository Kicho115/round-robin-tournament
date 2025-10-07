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

    auto result = tournamentDelegate->CreateTournament(tournament);
    crow::response response;
    
    if (result.has_value()) {
        response.code = crow::CREATED;
        response.add_header("location", result.value());
    } else {
        response.code = crow::INTERNAL_SERVER_ERROR;
        response.body = nlohmann::json{{"error", result.error()}}.dump();
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    }
    
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

crow::response TournamentController::UpdateTournament(const crow::request &request, const std::string& id) const {
    nlohmann::json updates = nlohmann::json::parse(request.body);
    
    // Get the tournamen that we want to update
    std::shared_ptr<domain::Tournament> oldTournament = tournamentDelegate->ReadById(id);
    
    if (!oldTournament) {
        crow::response response;
        response.code = crow::NOT_FOUND;
        response.body = R"({"error": "Tournament not found"})";
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    
    // Convert old tournament to JSON
    nlohmann::json oldTournamentJson = *oldTournament;
    
    // update only the fields that we want to update
    oldTournamentJson.merge_patch(updates);
    
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(oldTournamentJson);
    
    auto result = tournamentDelegate->UpdateTournament(id, tournament);

    crow::response response;
    
    if (result.has_value()) {
        response.code = crow::NO_CONTENT;
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    } else {
        response.code = crow::INTERNAL_SERVER_ERROR;
        response.body = nlohmann::json{{"error", result.error()}}.dump();
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    }
    
    return response;
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, GetById, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)