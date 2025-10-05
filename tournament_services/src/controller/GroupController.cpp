#include <regex>
#include <nlohmann/json.hpp>
#include <crow.h>

#include "configuration/RouteDefinition.hpp"
#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Utilities.hpp"

using nlohmann::json;

static constexpr auto JSON_CT = "application/json";

GroupController::GroupController(std::shared_ptr<IGroupDelegate> delegate)
: groupDelegate(std::move(delegate)) {}

crow::response GroupController::GetGroups(const crow::request&, const std::string& tid) const {
    if (!std::regex_match(tid, ID_VALUE)) return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    json body = groupDelegate->ReadByTournament(tid);
    crow::response res{crow::OK, body.dump()};
    res.add_header("content-type", JSON_CT);
    return res;
}

crow::response GroupController::CreateGroup(const crow::request& req, const std::string& tid) const {
    if (!std::regex_match(tid, ID_VALUE)) return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    if (!json::accept(req.body)) return crow::response{crow::BAD_REQUEST};

    json in = json::parse(req.body);
    std::string name = in.value("name", "");
    json teams = in.value("teams", json::array());

    auto created = groupDelegate->CreateGroup(tid, name, teams);
    if (!created.has_value()) {
        if (created.error() == "tournament_not_found") return crow::response{crow::NOT_FOUND, "tournament_not_found"};
        if (created.error() == "duplicate_group_name") return crow::response{crow::CONFLICT, "duplicate_group_name"};
        return crow::response{crow::UNPROCESSABLE, created.error()};
    }
    crow::response res{crow::CREATED};
    res.add_header("location", ("/tournaments/" + tid + "/groups/" + created.value()).c_str());
    return res;
}

crow::response GroupController::AddTeam(const crow::request& req, const std::string& tid, const std::string& gid) const {
    if (!std::regex_match(tid, ID_VALUE) || !std::regex_match(gid, ID_VALUE)) return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    if (!json::accept(req.body)) return crow::response{crow::BAD_REQUEST};

    auto in = json::parse(req.body);
    if (!in.contains("id") || !in.contains("name")) return crow::response{crow::BAD_REQUEST, "missing team"};

    auto r = groupDelegate->AddTeamToGroup(tid, gid, in.at("id").get<std::string>(), in.at("name").get<std::string>());
    if (!r.has_value()) {
        if (r.error()=="tournament_not_found")   return crow::response{crow::NOT_FOUND, "tournament_not_found"};
        if (r.error()=="group_not_found")        return crow::response{crow::NOT_FOUND, "group_not_found"};
        if (r.error()=="team_not_found")         return crow::response{crow::UNPROCESSABLE, "team_not_found"};
        if (r.error()=="team_already_in_group")  return crow::response{crow::UNPROCESSABLE, "team_already_in_group"};
        if (r.error()=="group_full")             return crow::response{crow::UNPROCESSABLE, "group_full"};
        return crow::response{crow::UNPROCESSABLE, r.error()};
    }
    return crow::response{crow::CREATED};
}

// Igual que haces en Team/Tournament, registra rutas con macro
REGISTER_ROUTE(GroupController, GetGroups,   "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, AddTeam,     "/tournaments/<string>/groups/<string>", "POST"_method)
