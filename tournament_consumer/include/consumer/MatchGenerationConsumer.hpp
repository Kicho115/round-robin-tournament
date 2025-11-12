#ifndef CONSUMER_MATCH_GENERATION_CONSUMER_HPP
#define CONSUMER_MATCH_GENERATION_CONSUMER_HPP

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"

class MatchGenerationConsumer {
    std::shared_ptr<IGroupRepository> groupRepo;
    std::shared_ptr<IMatchRepository> matchRepo;
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo;

public:
    MatchGenerationConsumer(
        std::shared_ptr<IGroupRepository> groupRepo,
        std::shared_ptr<IMatchRepository> matchRepo,
        std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo
    );

    // Handle team_added event
    void Handle(const nlohmann::json& eventJson);

private:
    void GenerateMatchesForGroup(const std::string& tournamentId, const std::string& groupId);
    bool ShouldGenerateMatches(const std::string& groupId, int maxTeamsPerGroup);
};

#endif // CONSUMER_MATCH_GENERATION_CONSUMER_HPP


