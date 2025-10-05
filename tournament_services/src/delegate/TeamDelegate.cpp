//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository) : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(id.data());
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team){

    return teamRepository->Create(team);
}



std::string_view TeamDelegate::SaveTeam(const domain::Team& team){
    // el repo respetará índice único de nombre → si hay duplicado, lanzará o retornará ""
    return teamRepository->Create(team);
}

std::expected<void, std::string> TeamDelegate::UpdateTeam(const domain::Team& team) const {
    // verifica existencia previa para poder devolver 404
    auto current = teamRepository->ReadById(team.Id);
    if (!current) return std::unexpected("team_not_found");

    teamRepository->Update(team);
    return {};
}