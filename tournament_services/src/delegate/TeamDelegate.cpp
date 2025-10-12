//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"
#include <utility>
#include <string>
#include <string_view>

std::string TeamDelegate::SaveTeam(const domain::Team& team) {
    // Si el repo devuelve std::string_view (clave), conviértelo explícitamente a std::string (C++17).
    auto id_sv = teamRepository->Create(team);
    return std::string{id_sv};
}

std::optional<std::string> TeamDelegate::UpdateTeam(const domain::Team& team) const {
    // verifica existencia previa para poder devolver 404
    auto current = teamRepository->ReadById(team.Id);
    if (!current) return std::make_optional<std::string>("team_not_found");

    teamRepository->Update(team);
    return std::nullopt; // OK
}
