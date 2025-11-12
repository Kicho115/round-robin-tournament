#ifndef SERVICES_IMATCH_DELEGATE_HPP
#define SERVICES_IMATCH_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

// DTO for match response with team names
struct MatchDTO {
    std::string matchId;
    std::string tournamentId;
    std::string groupId;
    
    struct TeamInfo {
        std::string id;
        std::string name;
    };
    
    TeamInfo home;
    TeamInfo visitor;
    
    std::string round;  // "regular" for round-robin
    
    struct ScoreInfo {
        int home;
        int visitor;
    };
    
    std::optional<ScoreInfo> score;
    
    nlohmann::json ToJson() const {
        nlohmann::json j;
        j["home"] = {{"id", home.id}, {"name", home.name}};
        j["visitor"] = {{"id", visitor.id}, {"name", visitor.name}};
        j["round"] = round;
        
        if (score.has_value()) {
            j["score"] = {
                {"home", score->home},
                {"visitor", score->visitor}
            };
        }
        
        return j;
    }
};

enum class MatchFilterType {
    All,
    Played,
    Pending
};

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;
    
    // Get all matches for a tournament with optional filter
    // Returns: nullopt on success, error message on failure
    virtual std::optional<std::string>
    GetMatches(std::string_view tournamentId, 
               MatchFilterType filter,
               std::vector<MatchDTO>& outMatches) = 0;
    
    // Get a specific match
    // Returns: nullopt on success, error message on failure
    virtual std::optional<std::string>
    GetMatch(std::string_view tournamentId,
             std::string_view matchId,
             MatchDTO& outMatch) = 0;
    
    // Update match score
    // Returns: nullopt on success, error message on failure
    virtual std::optional<std::string>
    UpdateScore(std::string_view tournamentId,
                std::string_view matchId,
                int homeScore,
                int awayScore) = 0;
};

#endif


