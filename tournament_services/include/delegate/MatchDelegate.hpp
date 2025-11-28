#ifndef SERVICES_MATCH_DELEGATE_HPP
#define SERVICES_MATCH_DELEGATE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "delegate/IMatchDelegate.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/ITournamentRepository.hpp"
#include "domain/Team.hpp"
#include "messaging/IEventBus.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepo;
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepo;
    std::shared_ptr<ITournamentRepository> tournamentRepo;
    std::shared_ptr<IEventBus> eventBus;

public:
    MatchDelegate(std::shared_ptr<IMatchRepository> matchRepo,
                  std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepo,
                  std::shared_ptr<ITournamentRepository> tournamentRepo,
                  std::shared_ptr<IEventBus> eventBus);

    std::optional<std::string>
    GetMatches(std::string_view tournamentId, 
               MatchFilterType filter,
               std::vector<MatchDTO>& outMatches) override;
    
    std::optional<std::string>
    GetMatch(std::string_view tournamentId,
             std::string_view matchId,
             MatchDTO& outMatch) override;
    
    std::optional<std::string>
    UpdateScore(std::string_view tournamentId,
                std::string_view matchId,
                int homeScore,
                int awayScore) override;

private:
    std::optional<MatchDTO> ConvertToDTO(const domain::Match& match);
};

#endif