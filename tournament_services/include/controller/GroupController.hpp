#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

#define JSON_CONTENT_TYPE  "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <vector>
#include <string>
#include <memory>

#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"

class GroupController {
    std::shared_ptr<IGroupDelegate> groupDelegate;

public:
    explicit GroupController(const std::shared_ptr<IGroupDelegate>& delegate);
    ~GroupController() = default;

    // GET /tournaments/<TOURNAMENT-ID>/groups
    crow::response GetGroups(const std::string& tournamentId);

    // GET /tournaments/<TOURNAMENT-ID>/groups/<GROUP-ID>
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);

    // POST /tournaments/<TOURNAMENT-ID>/groups
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);

    // PATCH /tournaments/<TOURNAMENT-ID>/groups/<GROUP-ID>
    crow::response UpdateGroup(const crow::request& request,
                               const std::string& tournamentId,
                               const std::string& groupId);

    // PATCH /tournaments/<TOURNAMENT-ID>/groups/<GROUP-ID>/teams
    crow::response UpdateTeams(const crow::request& request,
                               const std::string& tournamentId,
                               const std::string& groupId);

    // POST /tournaments/<TOURNAMENT-ID>/groups/<GROUP-ID>
    crow::response AddTeam(const crow::request& request,
                           const std::string& tournamentId,
                           const std::string& groupId);
};

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
