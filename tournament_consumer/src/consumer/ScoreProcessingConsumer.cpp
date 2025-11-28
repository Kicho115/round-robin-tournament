#include "consumer/ScoreProcessingConsumer.hpp"
#include "domain/Tournament.hpp"
#include "util/StandingsCalculator.hpp"
#include "util/KnockoutBracketBuilder.hpp"
#include <iostream>
#include <algorithm>

ScoreProcessingConsumer::ScoreProcessingConsumer(
    std::shared_ptr<IMatchRepository> matchRepo,
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo
) : matchRepo(std::move(matchRepo)),
    tournamentRepo(std::move(tournamentRepo)) {
}

void ScoreProcessingConsumer::Handle(const nlohmann::json& eventJson) {
    try {
        std::string tournamentId = eventJson.value("tournamentId", "");
        std::string matchId = eventJson.value("matchId", "");
        
        if (tournamentId.empty() || matchId.empty()) {
            std::cerr << "ScoreProcessingConsumer: Missing tournamentId or matchId" << std::endl;
            return;
        }
        
        std::cout << "ScoreProcessingConsumer: Processing score update for match " 
                  << matchId << " in tournament " << tournamentId << std::endl;
        
        // First, try to advance if it's a knockout match
        ProcessKnockoutAdvancement(tournamentId, matchId);

        // Then check if we need to transition phases or finish
        CheckTournamentCompletion(tournamentId);
        
    } catch (const std::exception& e) {
        std::cerr << "ScoreProcessingConsumer: Error handling event: " << e.what() << std::endl;
    }
}

void ScoreProcessingConsumer::ProcessKnockoutAdvancement(const std::string& tournamentId, const std::string& matchId) {
    auto matchPtr = matchRepo->FindByTournamentIdAndMatchId(tournamentId, matchId);
    if (!matchPtr) return;
    
    domain::Match& match = *matchPtr;
    
    if (match.Phase != domain::MatchPhase::Knockout) return;
    if (!match.IsScored()) return;
    
    // Determine winner
    int homeScore = *match.HomeScore;
    int awayScore = *match.AwayScore;
    std::string winnerId;
    
    if (homeScore > awayScore) winnerId = match.HomeTeamId;
    else if (awayScore > homeScore) winnerId = match.AwayTeamId;
    else {
        // Draw in knockout? Simple rule: Home advances (or random). 
        // Ideally should have penalties score. For now, Home wins on tie.
        winnerId = match.HomeTeamId;
    }
    
    // Construct placeholder string
    std::string placeholder = "W(" + match.HomeTeamId + "-" + match.AwayTeamId + ")";
    
    // Find future matches using this placeholder
    auto allMatches = matchRepo->FindByTournamentId(tournamentId); // Maybe filter pending?
    
    for (auto& mPtr : allMatches) {
        if (!mPtr) continue;
        bool modified = false;
        if (mPtr->HomeTeamId == placeholder) {
            mPtr->HomeTeamId = winnerId;
            modified = true;
        }
        if (mPtr->AwayTeamId == placeholder) {
            mPtr->AwayTeamId = winnerId;
            modified = true;
        }
        
        if (modified) {
            matchRepo->Update(*mPtr);
            std::cout << "ScoreProcessingConsumer: Advanced " << winnerId << " to match " << mPtr->Id << std::endl;
        }
    }
}

void ScoreProcessingConsumer::GeneratePlayoffs(const std::string& tournamentId, const std::vector<domain::Match>& rrMatches) {
    std::cout << "ScoreProcessingConsumer: Generating Playoffs for tournament " << tournamentId << std::endl;
    
    standings::StandingsCalculator calc;
    auto table = calc.Compute(rrMatches);
    
    // Assume Top 8. If fewer than 8 teams, logic handles it? 
    // The Builder requires 8 seeds for BuildTop8, or uses BuildTop8FromSeeds which checks size.
    // Let's use the table to get seeds.
    auto koMatches = KnockoutBracketBuilder::BuildTop8(table, tournamentId);
    
    if (koMatches.empty()) {
        std::cerr << "ScoreProcessingConsumer: Not enough teams for Playoffs or error generating bracket." << std::endl;
        return;
    }
    
    for (const auto& m : koMatches) {
        matchRepo->Create(m);
    }
    
    std::cout << "ScoreProcessingConsumer: Created " << koMatches.size() << " Playoff matches." << std::endl;
}

void ScoreProcessingConsumer::CheckTournamentCompletion(const std::string& tournamentId) {
    // Get all matches
    auto allMatchesPtrs = matchRepo->FindByTournamentId(tournamentId);
    
    std::vector<domain::Match> rrMatches;
    bool hasKnockout = false;
    bool allRRComplete = true;
    bool allKOComplete = true;
    
    for (const auto& mPtr : allMatchesPtrs) {
        if (!mPtr) continue;
        if (mPtr->Phase == domain::MatchPhase::Knockout) {
            hasKnockout = true;
            if (!mPtr->IsScored()) allKOComplete = false;
        } else {
            rrMatches.push_back(*mPtr);
            if (!mPtr->IsScored()) allRRComplete = false;
        }
    }
    
    if (!hasKnockout) {
        // We are in RR phase
        if (allRRComplete && !rrMatches.empty()) {
            // RR finished, start Playoffs
            GeneratePlayoffs(tournamentId, rrMatches);
        } else {
             std::cout << "ScoreProcessingConsumer: Round Robin in progress." << std::endl;
        }
    } else {
        // We are in KO phase
        if (allKOComplete) {
            std::cout << "ScoreProcessingConsumer: Tournament " << tournamentId << " is now COMPLETE!" << std::endl;
            // Mark tournament completed in Repo if field exists
        } else {
            std::cout << "ScoreProcessingConsumer: Playoffs in progress." << std::endl;
        }
    }
}

bool ScoreProcessingConsumer::AllMatchesCompleted(const std::string& tournamentId) {
    // This method might be redundant now with the new logic inside CheckTournamentCompletion
    // Keeping it if needed for other checks, but implementing via Repo
    int totalMatches = matchRepo->CountTotalMatchesByTournament(tournamentId);
    int completedMatches = matchRepo->CountCompletedMatchesByTournament(tournamentId);
    return totalMatches > 0 && completedMatches == totalMatches;
}
