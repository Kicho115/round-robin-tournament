#include "controller/MatchController.hpp"
#include <nlohmann/json.hpp>
#include <string>

MatchController::MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate)
    : matchDelegate(matchDelegate) {
}

crow::response MatchController::GetMatches(const crow::request& request, 
                                          const std::string& tournamentId) const {
    // Parse query parameter for filter
    MatchFilterType filter = MatchFilterType::All;
    auto showMatches = request.url_params.get("showMatches");
    if (showMatches) {
        std::string filterStr(showMatches);
        if (filterStr == "played") {
            filter = MatchFilterType::Played;
        } else if (filterStr == "pending") {
            filter = MatchFilterType::Pending;
        }
    }
    
    std::vector<MatchDTO> matches;
    auto error = matchDelegate->GetMatches(tournamentId, filter, matches);
    
    if (error.has_value()) {
        if (*error == "tournament_not_found") {
            return crow::response(crow::NOT_FOUND);
        }
        return crow::response(crow::INTERNAL_SERVER_ERROR);
    }
    
    // Convert to JSON array
    nlohmann::json jsonArray = nlohmann::json::array();
    for (const auto& match : matches) {
        jsonArray.push_back(match.ToJson());
    }
    
    crow::response response(crow::OK);
    response.set_header("content-type", "application/json");
    response.body = jsonArray.dump();
    return response;
}

crow::response MatchController::GetMatch(const std::string& tournamentId,
                                        const std::string& matchId) const {
    MatchDTO match;
    auto error = matchDelegate->GetMatch(tournamentId, matchId, match);
    
    if (error.has_value()) {
        if (*error == "match_not_found") {
            return crow::response(crow::NOT_FOUND);
        }
        return crow::response(crow::INTERNAL_SERVER_ERROR);
    }
    
    crow::response response(crow::OK);
    response.set_header("content-type", "application/json");
    response.body = match.ToJson().dump();
    return response;
}

crow::response MatchController::UpdateMatchScore(const crow::request& request,
                                                const std::string& tournamentId,
                                                const std::string& matchId) const {
    // Parse request body
    try {
        auto json = nlohmann::json::parse(request.body);
        
        if (!json.contains("score") || !json["score"].is_object()) {
            crow::response response(422);  // Unprocessable Entity
            response.set_header("content-type", "application/json");
            response.body = R"({"error": "Missing or invalid score object"})";
            return response;
        }
        
        auto scoreObj = json["score"];
        if (!scoreObj.contains("home") || !scoreObj.contains("visitor")) {
            crow::response response(422);  // Unprocessable Entity
            response.set_header("content-type", "application/json");
            response.body = R"({"error": "Missing home or visitor score"})";
            return response;
        }
        
        int homeScore = scoreObj["home"].get<int>();
        int awayScore = scoreObj["visitor"].get<int>();
        
        auto error = matchDelegate->UpdateScore(tournamentId, matchId, homeScore, awayScore);
        
        if (error.has_value()) {
            if (*error == "match_not_found") {
                return crow::response(crow::NOT_FOUND);
            } else if (*error == "invalid_score") {
                crow::response response(422);  // Unprocessable Entity
                response.set_header("content-type", "application/json");
                response.body = R"({"error": "Invalid score: scores must be non-negative"})";
                return response;
            } else if (*error == "database_error") {
                crow::response response(crow::INTERNAL_SERVER_ERROR);
                response.set_header("content-type", "application/json");
                response.body = R"({"error": "Database error"})";
                return response;
            }
            return crow::response(crow::INTERNAL_SERVER_ERROR);
        }
        
        return crow::response(crow::NO_CONTENT);
        
    } catch (const nlohmann::json::exception& e) {
        crow::response response(422);  // Unprocessable Entity
        response.set_header("content-type", "application/json");
        response.body = nlohmann::json{{"error", "Invalid JSON"}}.dump();
        return response;
    } catch (const std::exception& e) {
        return crow::response(crow::INTERNAL_SERVER_ERROR);
    }
}

// Register routes
#include "configuration/RouteDefinition.hpp"

REGISTER_ROUTE(MatchController, GetMatches, "/tournaments/<string>/matches", "GET"_method)
REGISTER_ROUTE(MatchController, GetMatch, "/tournaments/<string>/matches/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, UpdateMatchScore, "/tournaments/<string>/matches/<string>", "PATCH"_method)


