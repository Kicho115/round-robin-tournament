#ifndef CONSUMER_SCORE_PROCESSING_CONSUMER_HPP
#define CONSUMER_SCORE_PROCESSING_CONSUMER_HPP

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"
#include "domain/Match.hpp"

class ScoreProcessingConsumer {
    std::shared_ptr<IMatchRepository> matchRepo;
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo;

public:
    ScoreProcessingConsumer(
        std::shared_ptr<IMatchRepository> matchRepo,
        std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo
    );

    // Handle match_score_updated event
    void Handle(const nlohmann::json& eventJson);

private:
    void CheckTournamentCompletion(const std::string& tournamentId);
    void ProcessKnockoutAdvancement(const std::string& tournamentId, const std::string& matchId);
    void GeneratePlayoffs(const std::string& tournamentId, const std::vector<domain::Match>& rrMatches);
    bool AllMatchesCompleted(const std::string& tournamentId);
};

#endif // CONSUMER_SCORE_PROCESSING_CONSUMER_HPP
