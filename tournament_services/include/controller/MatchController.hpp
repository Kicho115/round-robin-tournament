#ifndef SERVICES_MATCH_CONTROLLER_HPP
#define SERVICES_MATCH_CONTROLLER_HPP

#include <memory>
#include <string>
#include <crow.h>

#include "delegate/IMatchDelegate.hpp"

class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;

public:
    explicit MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate);

    // GET /tournaments/<tournamentId>/matches
    [[nodiscard]] crow::response GetMatches(const crow::request& request, 
                                           const std::string& tournamentId) const;

    // GET /tournaments/<tournamentId>/matches/<matchId>
    [[nodiscard]] crow::response GetMatch(const std::string& tournamentId,
                                         const std::string& matchId) const;

    // PATCH /tournaments/<tournamentId>/matches/<matchId>
    [[nodiscard]] crow::response UpdateMatchScore(const crow::request& request,
                                                  const std::string& tournamentId,
                                                  const std::string& matchId) const;
};

#endif
