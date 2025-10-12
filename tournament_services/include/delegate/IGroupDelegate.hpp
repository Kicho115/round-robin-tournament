#ifndef SERVICE_IGROUP_DELEGATE_HPP
#define SERVICE_IGROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>

#include "domain/Group.hpp"
#include "domain/Team.hpp"

class IGroupDelegate {
public:
    virtual ~IGroupDelegate() = default;

    // Crea el grupo; si éxito => outGroupId con el ID y retorna nullopt.
    // Si error => retorna optional con el mensaje ("tournament_not_found", "duplicate_group_name", "group_limit_reached", etc.)
    virtual std::optional<std::string>
    CreateGroup(std::string_view tournamentId, const domain::Group& group, std::string& outGroupId) = 0;

    // Lee todos los grupos del torneo; si éxito => llena outGroups y retorna nullopt; si error => mensaje.
    virtual std::optional<std::string>
    GetGroups(std::string_view tournamentId, std::vector<std::shared_ptr<domain::Group>>& outGroups) = 0;

    // Lee un grupo específico; si éxito => outGroup con el grupo; si error => mensaje ("tournament_not_found", "group_not_found")
    virtual std::optional<std::string>
    GetGroup(std::string_view tournamentId, std::string_view groupId, std::shared_ptr<domain::Group>& outGroup) = 0;

    // Actualiza un grupo; nullopt = OK; valor = error
    virtual std::optional<std::string>
    UpdateGroup(std::string_view tournamentId, const domain::Group& group) = 0;

    // Elimina un grupo; nullopt = OK; valor = error
    virtual std::optional<std::string>
    RemoveGroup(std::string_view tournamentId, std::string_view groupId) = 0;

    // Reemplaza/actualiza equipos del grupo; nullopt = OK; valor = error
    virtual std::optional<std::string>
    UpdateTeams(std::string_view tournamentId, std::string_view groupId, const std::vector<domain::Team>& teams) = 0;
};

#endif /* SERVICE_IGROUP_DELEGATE_HPP */
