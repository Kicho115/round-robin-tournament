#include "consumer/MatchGenerationConsumer.hpp"
#include "util/RoundRobinGenerator.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"
#include <iostream>

MatchGenerationConsumer::MatchGenerationConsumer(
    std::shared_ptr<IGroupRepository> groupRepo,
    std::shared_ptr<IMatchRepository> matchRepo,
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo
) : groupRepo(std::move(groupRepo)),
    matchRepo(std::move(matchRepo)),
    tournamentRepo(std::move(tournamentRepo)) {
}

void MatchGenerationConsumer::Handle(const nlohmann::json& eventJson) {
    try {
        std::string tournamentId = eventJson.value("tournamentId", "");
        std::string groupId = eventJson.value("groupId", "");
        
        if (tournamentId.empty() || groupId.empty()) {
            std::cerr << "MatchGenerationConsumer: Missing tournamentId or groupId" << std::endl;
            return;
        }
        
        // Get tournament to know max teams per group
        auto tournament = tournamentRepo->ReadById(tournamentId);
        if (!tournament) {
            std::cerr << "MatchGenerationConsumer: Tournament not found: " << tournamentId << std::endl;
            return;
        }
        
        int maxTeamsPerGroup = tournament->Format().MaxTeamsPerGroup();
        
        // Check if we should generate matches
        if (ShouldGenerateMatches(groupId, maxTeamsPerGroup)) {
            GenerateMatchesForGroup(tournamentId, groupId);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "MatchGenerationConsumer: Error handling event: " << e.what() << std::endl;
    }
}

bool MatchGenerationConsumer::ShouldGenerateMatches(const std::string& groupId, int maxTeamsPerGroup) {
    // Check if group has reached maximum teams
    int teamCount = groupRepo->CountTeamsInGroup(groupId);
    
    if (teamCount < maxTeamsPerGroup) {
        return false;  // Not all teams added yet
    }
    
    // Check if matches already exist for this group
    auto existingMatches = matchRepo->FindByGroupId(groupId);
    if (!existingMatches.empty()) {
        std::cout << "MatchGenerationConsumer: Matches already exist for group " << groupId << std::endl;
        return false;  // Matches already generated
    }
    
    return true;
}

void MatchGenerationConsumer::GenerateMatchesForGroup(const std::string& tournamentId, const std::string& groupId) {
    try {
        // Get group with teams
        std::shared_ptr<domain::Group> group;
        auto error = groupRepo->GetGroup(tournamentId, groupId, group);
        
        if (error.has_value() || !group) {
            std::cerr << "MatchGenerationConsumer: Failed to get group: " << groupId << std::endl;
            return;
        }
        
        // Generate matches using RoundRobinGenerator
        auto matches = RoundRobinGenerator::Generate(group->Teams(), tournamentId, groupId);
        
        std::cout << "MatchGenerationConsumer: Generated " << matches.size() 
                  << " matches for group " << groupId << std::endl;
        
        // Persist matches
        for (const auto& match : matches) {
            try {
                matchRepo->Create(match);
            } catch (const std::exception& e) {
                std::cerr << "MatchGenerationConsumer: Failed to create match: " << e.what() << std::endl;
            }
        }
        
        std::cout << "MatchGenerationConsumer: Successfully created matches for group " << groupId << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "MatchGenerationConsumer: Error generating matches: " << e.what() << std::endl;
    }
}


