#include "delegate/MatchDelegate.hpp"
#include "domain/Match.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include <nlohmann/json.hpp>

MatchDelegate::MatchDelegate(std::shared_ptr<IMatchRepository> matchRepo,
                             std::shared_ptr<ITeamRepository> teamRepo,
                             std::shared_ptr<ITournamentRepository> tournamentRepo,
                             std::shared_ptr<IEventBus> eventBus)
    : matchRepo(std::move(matchRepo)),
      teamRepo(std::move(teamRepo)),
      tournamentRepo(std::move(tournamentRepo)),
      eventBus(std::move(eventBus)) {
}

std::optional<MatchDTO> MatchDelegate::ConvertToDTO(const domain::Match& match) {
    MatchDTO dto;
    dto.matchId = match.Id;
    dto.tournamentId = match.TournamentId;
    dto.groupId = match.GroupId;
    
    // Fetch team names
    auto homeTeam = teamRepo->ReadById(match.HomeTeamId);
    auto awayTeam = teamRepo->ReadById(match.AwayTeamId);
    
    if (!homeTeam || !awayTeam) {
        return std::nullopt;  // Team not found
    }
    
    dto.home.id = match.HomeTeamId;
    dto.home.name = homeTeam->Name;
    dto.visitor.id = match.AwayTeamId;
    dto.visitor.name = awayTeam->Name;
    
    // Map phase to "regular" for round-robin
    dto.round = "regular";
    
    // Add score if present
    if (match.IsScored()) {
        MatchDTO::ScoreInfo score;
        score.home = *match.HomeScore;
        score.visitor = *match.AwayScore;
        dto.score = score;
    }
    
    return dto;
}

std::optional<std::string>
MatchDelegate::GetMatches(std::string_view tournamentId, 
                         MatchFilterType filter,
                         std::vector<MatchDTO>& outMatches) {
    // Check if tournament exists
    auto tournament = tournamentRepo->ReadById(std::string(tournamentId));
    if (!tournament) {
        return "tournament_not_found";
    }
    
    // Convert filter type
    MatchFilter repoFilter = MatchFilter::All;
    switch (filter) {
        case MatchFilterType::Played:
            repoFilter = MatchFilter::Played;
            break;
        case MatchFilterType::Pending:
            repoFilter = MatchFilter::Pending;
            break;
        default:
            repoFilter = MatchFilter::All;
            break;
    }
    
    // Fetch matches from repository
    auto matches = matchRepo->FindByTournamentId(tournamentId, repoFilter);
    
    // Convert to DTOs
    outMatches.clear();
    for (const auto& match : matches) {
        auto dto = ConvertToDTO(*match);
        if (dto.has_value()) {
            outMatches.push_back(*dto);
        }
    }
    
    return std::nullopt;  // Success
}

std::optional<std::string>
MatchDelegate::GetMatch(std::string_view tournamentId,
                       std::string_view matchId,
                       MatchDTO& outMatch) {
    // Fetch match
    auto match = matchRepo->FindByTournamentIdAndMatchId(tournamentId, matchId);
    if (!match) {
        return "match_not_found";
    }
    
    // Convert to DTO
    auto dto = ConvertToDTO(*match);
    if (!dto.has_value()) {
        return "team_not_found";
    }
    
    outMatch = *dto;
    return std::nullopt;  // Success
}

std::optional<std::string>
MatchDelegate::UpdateScore(std::string_view tournamentId,
                          std::string_view matchId,
                          int homeScore,
                          int awayScore) {
    // Validate scores (non-negative)
    if (homeScore < 0 || awayScore < 0) {
        return "invalid_score";
    }
    
    // Check if match exists
    auto match = matchRepo->FindByTournamentIdAndMatchId(tournamentId, matchId);
    if (!match) {
        return "match_not_found";
    }
    
    // Update score
    bool success = matchRepo->UpdateScore(matchId, homeScore, awayScore);
    if (!success) {
        return "database_error";
    }
    
    // Publish event
    if (eventBus) {
        nlohmann::json payload = {
            {"event", "match_score_updated"},
            {"tournamentId", std::string(tournamentId)},
            {"matchId", std::string(matchId)},
            {"groupId", match->GroupId},
            {"homeTeamId", match->HomeTeamId},
            {"awayTeamId", match->AwayTeamId},
            {"homeScore", homeScore},
            {"awayScore", awayScore}
        };
        eventBus->Publish("match.score_updated", payload);
    }
    
    return std::nullopt;  // Success
}


