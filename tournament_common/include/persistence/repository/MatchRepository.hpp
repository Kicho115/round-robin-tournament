#ifndef COMMON_MATCH_REPOSITORY_HPP
#define COMMON_MATCH_REPOSITORY_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <pqxx/pqxx>

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "domain/Match.hpp"

class MatchRepository : public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    explicit MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);

    // IRepository methods
    std::shared_ptr<domain::Match> ReadById(std::string id) override;
    std::string Create(const domain::Match& entity) override;
    std::string Update(const domain::Match& entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Match>> ReadAll() override;

    // IMatchRepository methods
    std::vector<std::shared_ptr<domain::Match>>
    FindByTournamentId(std::string_view tournamentId, MatchFilter filter = MatchFilter::All) override;

    std::shared_ptr<domain::Match>
    FindByTournamentIdAndMatchId(std::string_view tournamentId, std::string_view matchId) override;

    bool UpdateScore(std::string_view matchId, int homeScore, int awayScore) override;

    int CountCompletedMatchesByTournament(std::string_view tournamentId) override;

    int CountTotalMatchesByTournament(std::string_view tournamentId) override;

    std::vector<std::shared_ptr<domain::Match>>
    FindByGroupId(std::string_view groupId) override;

private:
    std::shared_ptr<domain::Match> ParseMatchFromRow(const pqxx::row& row);
};

#endif


