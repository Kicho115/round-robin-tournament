#include "consumer/ScoreProcessingConsumer.hpp"
#include "domain/Tournament.hpp"
#include <iostream>

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
        
        // Check if tournament is complete
        CheckTournamentCompletion(tournamentId);
        
    } catch (const std::exception& e) {
        std::cerr << "ScoreProcessingConsumer: Error handling event: " << e.what() << std::endl;
    }
}

bool ScoreProcessingConsumer::AllMatchesCompleted(const std::string& tournamentId) {
    int totalMatches = matchRepo->CountTotalMatchesByTournament(tournamentId);
    int completedMatches = matchRepo->CountCompletedMatchesByTournament(tournamentId);
    
    std::cout << "ScoreProcessingConsumer: Tournament " << tournamentId 
              << " has " << completedMatches << " of " << totalMatches << " matches completed" << std::endl;
    
    return totalMatches > 0 && completedMatches == totalMatches;
}

void ScoreProcessingConsumer::CheckTournamentCompletion(const std::string& tournamentId) {
    try {
        if (!AllMatchesCompleted(tournamentId)) {
            return;  // Tournament not complete yet
        }
        
        std::cout << "ScoreProcessingConsumer: All matches completed for tournament " << tournamentId << std::endl;
        
        // Get tournament and mark as completed
        auto tournament = tournamentRepo->ReadById(tournamentId);
        if (!tournament) {
            std::cerr << "ScoreProcessingConsumer: Tournament not found: " << tournamentId << std::endl;
            return;
        }
        
        // Note: We would need to add an isCompleted field to the Tournament domain model
        // For now, we just log that the tournament is complete
        std::cout << "ScoreProcessingConsumer: Tournament " << tournamentId << " is now COMPLETE!" << std::endl;
        
        // In a real implementation, you would:
        // 1. Update tournament.isCompleted = true
        // 2. Call tournamentRepo->Update(tournament)
        // 3. Publish a "tournament.completed" event
        
    } catch (const std::exception& e) {
        std::cerr << "ScoreProcessingConsumer: Error checking tournament completion: " << e.what() << std::endl;
    }
}


