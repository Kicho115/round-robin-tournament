#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>          // <— sustituye <expected>
#include "domain/Team.hpp"

class ITeamDelegate {
public:
    virtual ~ITeamDelegate() = default;

    virtual std::shared_ptr<domain::Team> GetTeam(const std::string& id) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> GetAllTeams() = 0;

    // Evitar vistas colgantes: regresar std::string con el id creado o "" si hubo duplicado
    virtual std::string SaveTeam(const domain::Team& team) = 0;

    // C++17: nullopt = OK; string = mensaje de error ("team_not_found", etc.)
    virtual std::optional<std::string> UpdateTeam(const domain::Team& team) const = 0;
};
