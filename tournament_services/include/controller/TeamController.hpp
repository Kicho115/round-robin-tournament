//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <memory>
#include <regex>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/ITeamDelegate.hpp"

// Anclado y como inline para evitar múltiples copias pesadas por TU
inline const std::regex ID_VALUE{R"(^[A-Za-z0-9-]+$)"};

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;

public:
    explicit TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate);

    [[nodiscard]] crow::response getTeam(const std::string& teamId) const;
    [[nodiscard]] crow::response getAllTeams() const;

    // Ya lo usas en el .cpp, debe existir aquí:
    [[nodiscard]] crow::response SaveTeam(const crow::request& request) const;
    [[nodiscard]] crow::response UpdateTeam(const crow::request& request,
                                            const std::string& teamId) const;
};

#endif // RESTAPI_TEAM_CONTROLLER_HPP
