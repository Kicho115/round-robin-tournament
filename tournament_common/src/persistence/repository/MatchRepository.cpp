#include "persistence/repository/MatchRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

MatchRepository::MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider)
    : connectionProvider(connectionProvider) {
}

std::shared_ptr<domain::Match> MatchRepository::ParseMatchFromRow(const pqxx::row& row) {
    try {
        nlohmann::json doc = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>();
        
        match->Id = row["id"].c_str();
        match->TournamentId = doc.value("tournamentId", std::string{});
        match->GroupId = doc.value("groupId", std::string{});
        match->HomeTeamId = doc.value("homeTeamId", std::string{});
        match->AwayTeamId = doc.value("awayTeamId", std::string{});
        match->Phase = domain::PhaseFromString(doc.value("phase", std::string{"RR"}));
        match->Round = doc.value("round", 0);
        
        if (doc.contains("homeScore") && !doc["homeScore"].is_null()) {
            match->HomeScore = doc["homeScore"].get<int>();
        }
        if (doc.contains("awayScore") && !doc["awayScore"].is_null()) {
            match->AwayScore = doc["awayScore"].get<int>();
        }
        
        return match;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

std::shared_ptr<domain::Match> MatchRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    try {
        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec(
            "SELECT * FROM matches WHERE id = $1::uuid",
            pqxx::params{id}
        );
        tx.commit();
        
        if (result.empty()) {
            return nullptr;
        }
        
        return ParseMatchFromRow(result[0]);
    } catch (const pqxx::data_exception& e) {
        return nullptr;
    }
}

std::string MatchRepository::Create(const domain::Match& entity) {
    nlohmann::json doc;
    doc["tournamentId"] = entity.TournamentId;
    doc["groupId"] = entity.GroupId;
    doc["homeTeamId"] = entity.HomeTeamId;
    doc["awayTeamId"] = entity.AwayTeamId;
    doc["phase"] = domain::ToString(entity.Phase);
    doc["round"] = entity.Round;
    
    if (entity.HomeScore.has_value()) {
        doc["homeScore"] = *entity.HomeScore;
    }
    if (entity.AwayScore.has_value()) {
        doc["awayScore"] = *entity.AwayScore;
    }
    
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"insert_match"}, doc.dump());
    tx.commit();
    
    return result[0]["id"].c_str();
}

std::string MatchRepository::Update(const domain::Match& entity) {
    nlohmann::json doc;
    doc["tournamentId"] = entity.TournamentId;
    doc["groupId"] = entity.GroupId;
    doc["homeTeamId"] = entity.HomeTeamId;
    doc["awayTeamId"] = entity.AwayTeamId;
    doc["phase"] = domain::ToString(entity.Phase);
    doc["round"] = entity.Round;
    
    if (entity.HomeScore.has_value()) {
        doc["homeScore"] = *entity.HomeScore;
    }
    if (entity.AwayScore.has_value()) {
        doc["awayScore"] = *entity.AwayScore;
    }
    
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    tx.exec(pqxx::prepped{"update_match"}, pqxx::params{entity.Id, doc.dump()});
    tx.commit();
    
    return entity.Id;
}

void MatchRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    tx.exec("DELETE FROM matches WHERE id = $1::uuid", pqxx::params{id});
    tx.commit();
}

std::vector<std::shared_ptr<domain::Match>> MatchRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Match>> matches;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec("SELECT * FROM matches");
    tx.commit();
    
    for (const auto& row : result) {
        auto match = ParseMatchFromRow(row);
        if (match) {
            matches.push_back(match);
        }
    }
    
    return matches;
}

std::vector<std::shared_ptr<domain::Match>>
MatchRepository::FindByTournamentId(std::string_view tournamentId, MatchFilter filter) {
    std::vector<std::shared_ptr<domain::Match>> matches;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    pqxx::work tx(*(connection->connection));
    pqxx::result result;
    
    switch (filter) {
        case MatchFilter::Played:
            result = tx.exec(pqxx::prepped{"select_matches_by_tournament_played"}, 
                           pqxx::params{std::string(tournamentId)});
            break;
        case MatchFilter::Pending:
            result = tx.exec(pqxx::prepped{"select_matches_by_tournament_pending"}, 
                           pqxx::params{std::string(tournamentId)});
            break;
        default:
            result = tx.exec(pqxx::prepped{"select_matches_by_tournament"}, 
                           pqxx::params{std::string(tournamentId)});
            break;
    }
    
    tx.commit();
    
    for (const auto& row : result) {
        auto match = ParseMatchFromRow(row);
        if (match) {
            matches.push_back(match);
        }
    }
    
    return matches;
}

std::shared_ptr<domain::Match>
MatchRepository::FindByTournamentIdAndMatchId(std::string_view tournamentId, std::string_view matchId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    try {
        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec(
            pqxx::prepped{"select_match_by_tournament_and_id"},
            pqxx::params{std::string(tournamentId), std::string(matchId)}
        );
        tx.commit();
        
        if (result.empty()) {
            return nullptr;
        }
        
        return ParseMatchFromRow(result[0]);
    } catch (const std::exception& e) {
        return nullptr;
    }
}

bool MatchRepository::UpdateScore(std::string_view matchId, int homeScore, int awayScore) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    try {
        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec(
            pqxx::prepped{"update_match_score"},
            pqxx::params{std::string(matchId), homeScore, awayScore}
        );
        tx.commit();
        
        return result.affected_rows() > 0;
    } catch (const std::exception& e) {
        return false;
    }
}

int MatchRepository::CountCompletedMatchesByTournament(std::string_view tournamentId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    try {
        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec(
            pqxx::prepped{"count_completed_matches_by_tournament"},
            pqxx::params{std::string(tournamentId)}
        );
        tx.commit();
        
        if (!result.empty()) {
            return result[0][0].as<int>();
        }
        return 0;
    } catch (const std::exception& e) {
        return 0;
    }
}

int MatchRepository::CountTotalMatchesByTournament(std::string_view tournamentId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    try {
        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec(
            pqxx::prepped{"count_total_matches_by_tournament"},
            pqxx::params{std::string(tournamentId)}
        );
        tx.commit();
        
        if (!result.empty()) {
            return result[0][0].as<int>();
        }
        return 0;
    } catch (const std::exception& e) {
        return 0;
    }
}

std::vector<std::shared_ptr<domain::Match>>
MatchRepository::FindByGroupId(std::string_view groupId) {
    std::vector<std::shared_ptr<domain::Match>> matches;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    
    try {
        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec(
            pqxx::prepped{"select_matches_by_group"},
            pqxx::params{std::string(groupId)}
        );
        tx.commit();
        
        for (const auto& row : result) {
            auto match = ParseMatchFromRow(row);
            if (match) {
                matches.push_back(match);
            }
        }
    } catch (const std::exception& e) {
        // Return empty vector on error
    }
    
    return matches;
}


