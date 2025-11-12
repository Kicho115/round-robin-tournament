#ifndef COMMON_IMATCH_REPOSITORY_HPP
#define COMMON_IMATCH_REPOSITORY_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "persistence/repository/IRepository.hpp"
#include "domain/Match.hpp"

enum class MatchFilter {
    All,
    Played,
    Pending
};

class IMatchRepository : public IRepository<domain::Match, std::string> {
public:
    virtual ~IMatchRepository() = default;

    // Find matches by tournament ID with optional filter
    virtual std::vector<std::shared_ptr<domain::Match>>
    FindByTournamentId(std::string_view tournamentId, MatchFilter filter = MatchFilter::All) = 0;

    // Find a specific match by tournament and match ID
    virtual std::shared_ptr<domain::Match>
    FindByTournamentIdAndMatchId(std::string_view tournamentId, std::string_view matchId) = 0;

    // Update score for a match
    virtual bool UpdateScore(std::string_view matchId, int homeScore, int awayScore) = 0;

    // Count completed matches in a tournament
    virtual int CountCompletedMatchesByTournament(std::string_view tournamentId) = 0;

    // Count total matches in a tournament
    virtual int CountTotalMatchesByTournament(std::string_view tournamentId) = 0;

    // Find matches by group ID
    virtual std::vector<std::shared_ptr<domain::Match>>
    FindByGroupId(std::string_view groupId) = 0;
};

#endif 
