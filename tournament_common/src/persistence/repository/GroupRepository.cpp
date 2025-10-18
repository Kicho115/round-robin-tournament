//
// Created by root on 9/27/25.
//

#include "persistence/repository/GroupRepository.hpp"

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using std::string;
using std::string_view;
using std::shared_ptr;
using std::make_shared;
using std::vector;

static nlohmann::json make_group_document(const domain::Group& g) {
    nlohmann::json j;
    j["name"] = g.Name();
    return j;
}

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& cp)
    : connectionProvider(cp) {}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*pg->connection);

    auto rs = tx.exec(
        pqxx::zview{
            "SELECT id, document->>'name' AS name, tournament_id "
            "FROM groups WHERE id = $1 LIMIT 1;"
        },
        pqxx::params{id.c_str()}
    );
    tx.commit();

    if (rs.empty()) return nullptr;

    auto g = std::make_shared<domain::Group>();
    g->Id()           = rs[0]["id"].c_str();
    g->Name()         = rs[0]["name"].c_str();
    g->TournamentId() = rs[0]["tournament_id"].c_str();
    return g;
}

std::string GroupRepository::Create(const domain::Group& entity) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);
    const nlohmann::json groupBody = make_group_document(entity);

    pqxx::work tx(*pg->connection);
    pqxx::result rs;

    if (entity.Id().empty()) {
        rs = tx.exec(
            pqxx::zview{
                "INSERT INTO groups (name, tournament_id, document) "
                "VALUES ($1, $2, $3::jsonb) "
                "RETURNING id;"
            },
            pqxx::params{
                entity.Name().c_str(),
                entity.TournamentId().c_str(),
                groupBody.dump().c_str()
            }
        );
    } else {
        rs = tx.exec(
            pqxx::zview{
                "INSERT INTO groups (id, name, tournament_id, document) "
                "VALUES ($1, $2, $3, $4::jsonb) "
                "RETURNING id;"
            },
            pqxx::params{
                entity.Id().c_str(),
                entity.Name().c_str(),
                entity.TournamentId().c_str(),
                groupBody.dump().c_str()
            }
        );
    }

    tx.commit();
    return rs.empty() ? std::string{} : std::string(rs[0]["id"].c_str());
}

std::string GroupRepository::Update(const domain::Group& entity) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);
    const nlohmann::json groupBody = make_group_document(entity);

    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{
            "UPDATE groups "
            "SET name = $2, tournament_id = $3, document = $4::jsonb "
            "WHERE id = $1 "
            "RETURNING id;"
        },
        pqxx::params{
            entity.Id().c_str(),
            entity.Name().c_str(),
            entity.TournamentId().c_str(),
            groupBody.dump().c_str()
        }
    );
    tx.commit();

    return rs.empty() ? std::string{} : std::string(rs[0]["id"].c_str());
}

void GroupRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*pg->connection);
    tx.exec(
        pqxx::zview{"DELETE FROM groups WHERE id = $1;"},
        pqxx::params{id.c_str()}
    );
    tx.commit();
}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::ReadAll() {
    vector<shared_ptr<domain::Group>> groups;

    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    pqxx::result rs = tx.exec(
        "SELECT id, document->>'name' AS name, tournament_id "
        "FROM groups ORDER BY id;"
    );
    tx.commit();

    groups.reserve(rs.size());
    for (const auto& row : rs) {
        auto g = std::make_shared<domain::Group>();
        g->Id()           = row["id"].c_str();
        g->Name()         = row["name"].c_str();
        g->TournamentId() = row["tournament_id"].c_str();
        groups.emplace_back(std::move(g));
    }

    return groups;
}

// Wrappers usados por GroupDelegate
std::optional<std::string>
GroupRepository::GetGroups(std::string_view tournamentId,
                           std::vector<std::shared_ptr<domain::Group>>& outGroups) {
    outGroups = FindByTournamentId(tournamentId);
    return std::nullopt;
}

std::optional<std::string>
GroupRepository::GetGroup(std::string_view tournamentId,
                          std::string_view groupId,
                          std::shared_ptr<domain::Group>& outGroup) {
    outGroup = FindByTournamentIdAndGroupId(tournamentId, groupId);
    return std::nullopt;
}

// Helpers internos también expuestos
std::vector<std::shared_ptr<domain::Group>>
GroupRepository::FindByTournamentId(std::string_view tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{
            "SELECT id, document->>'name' AS name, tournament_id "
            "FROM groups "
            "WHERE tournament_id = $1 "
            "ORDER BY id;"
        },
        pqxx::params{tournamentId.data()}
    );
    tx.commit();

    std::vector<std::shared_ptr<domain::Group>> groups;
    groups.reserve(rs.size());

    for (const auto& row : rs) {
        auto g = std::make_shared<domain::Group>();
        g->Id()           = row["id"].c_str();
        g->Name()         = row["name"].c_str();
        g->TournamentId() = row["tournament_id"].c_str();
        groups.emplace_back(std::move(g));
    }

    return groups;
}

std::shared_ptr<domain::Group>
GroupRepository::FindByTournamentIdAndGroupId(std::string_view tournamentId,
                                              std::string_view groupId) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{
            "SELECT id, document->>'name' AS name, tournament_id "
            "FROM groups "
            "WHERE tournament_id = $1 AND id = $2 "
            "LIMIT 1;"
        },
        pqxx::params{tournamentId.data(), groupId.data()}
    );
    tx.commit();

    if (rs.empty()) return nullptr;

    auto g = std::make_shared<domain::Group>();
    g->Id()           = rs[0]["id"].c_str();
    g->Name()         = rs[0]["name"].c_str();
    g->TournamentId() = rs[0]["tournament_id"].c_str();
    return g;
}

std::shared_ptr<domain::Group>
GroupRepository::FindByTournamentIdAndTeamId(std::string_view tournamentId,
                                             std::string_view teamId) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{
            "SELECT g.id, g.document->>'name' AS name, g.tournament_id "
            "FROM groups g "
            "JOIN group_teams gt ON gt.group_id = g.id "
            "WHERE g.tournament_id = $1 AND gt.team_id = $2 "
            "LIMIT 1;"
        },
        pqxx::params{tournamentId.data(), teamId.data()}
    );
    tx.commit();

    if (rs.empty()) return nullptr;

    auto g = std::make_shared<domain::Group>();
    g->Id()           = rs[0]["id"].c_str();
    g->Name()         = rs[0]["name"].c_str();
    g->TournamentId() = rs[0]["tournament_id"].c_str();
    return g;
}

void GroupRepository::UpdateGroupAddTeam(std::string_view groupId,
                                         const std::shared_ptr<domain::Team>& team) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    tx.exec(
        pqxx::zview{
            "INSERT INTO group_teams (group_id, team_id, team_name) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT DO NOTHING;"
        },
        pqxx::params{groupId.data(), team->Id.c_str(), team->Name.c_str()}
    );
    tx.commit();
}

bool GroupRepository::ExistsGroupForTournament(std::string_view tid) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{"SELECT 1 FROM groups WHERE tournament_id=$1 LIMIT 1;"},
        pqxx::params{tid.data()}
    );
    tx.commit();
    return !rs.empty();
}

int GroupRepository::GroupsCountForTournament(std::string_view tid) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{"SELECT COUNT(*) AS cnt FROM groups WHERE tournament_id=$1;"},
        pqxx::params{tid.data()}
    );
    tx.commit();
    return rs.empty() ? 0 : rs[0]["cnt"].as<int>(0);
}

int GroupRepository::CountTeamsInGroup(std::string_view groupId) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{
            "SELECT COUNT(*) AS cnt FROM group_teams WHERE group_id = $1;"
        },
        pqxx::params{ groupId.data() }
    );
    tx.commit();

    return rs.empty() ? 0 : rs[0]["cnt"].as<int>(0);
}

std::vector<domain::Team>
GroupRepository::GetTeamsOfGroup(std::string_view groupId) {
    auto pooled = connectionProvider->Connection();
    auto* pg = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*pg->connection);
    auto rs = tx.exec(
        pqxx::zview{
            "SELECT t.id, t.name "
            "FROM group_teams gt "
            "JOIN teams t ON t.id = gt.team_id "
            "WHERE gt.group_id = $1 "
            "ORDER BY t.name;"
        },
        pqxx::params{groupId.data()}
    );
    tx.commit();

    std::vector<domain::Team> out;
    out.reserve(rs.size());
    for (const auto& row : rs) {
        domain::Team t{};
        t.Id   = row["id"].c_str();
        t.Name = row["name"].c_str();
        out.emplace_back(std::move(t));
    }
    return out;
}
