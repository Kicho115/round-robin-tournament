// Created by developer on 8/22/25.

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <memory>
#include <regex>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/ITeamDelegate.hpp"

// Patrón simple para IDs (coincidir con otros controladores/proyectos)
inline const std::regex ID_VALUE{R"(^[A-Za-z0-9-]+$)"};

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;

public:
    explicit TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate);

    // GET /teams/<id>
    [[nodiscard]] crow::response getTeam(const std::string& teamId) const;

    // GET /teams  (firma que usan los tests)
    [[nodiscard]] crow::response GetTeams(const crow::request& request) const;

    // (helper opcional si quieres llamarlo desde router)
    [[nodiscard]] crow::response getAllTeams() const;

    // POST /teams
    [[nodiscard]] crow::response SaveTeam(const crow::request& request) const;

    // PATCH /teams/<id>
    [[nodiscard]] crow::response UpdateTeam(const crow::request& request,
                                            const std::string& teamId) const;
};

#endif // RESTAPI_TEAM_CONTROLLER_HPP
