#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "delegate/ITeamDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"

class TeamDelegate : public ITeamDelegate {
public:
    explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository)
        : teamRepository(std::move(repository)) {}

    std::vector<std::shared_ptr<domain::Team>> GetAllTeams() override {
        return teamRepository->ReadAll();
    }

    std::shared_ptr<domain::Team> GetTeam(const std::string& id) override {
        return teamRepository->ReadById(id);
    }

    // Devuelve id creado o "" si duplicado (el repo ya aplica unicidad)
    std::string SaveTeam(const domain::Team& team) override;

    // nullopt = OK; string = error ("team_not_found", …)
    std::optional<std::string> UpdateTeam(const domain::Team& team) const override;

private:
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;
};
