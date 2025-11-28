#include "util/KnockoutBracketBuilder.hpp"

using domain::Match;
using domain::MatchPhase;

static Match make_match(const std::string& t,
                        const std::string& h,
                        const std::string& a,
                        int round) {
    Match m;
    m.TournamentId = t;
    m.GroupId      = "";
    m.HomeTeamId   = h;
    m.AwayTeamId   = a;
    m.Phase        = MatchPhase::Knockout;
    m.Round        = round;
    return m;
}

std::vector<Match>
KnockoutBracketBuilder::BuildTop8(const std::vector<standings::Row>& table,
                                  const std::string& tournamentId) {
    std::vector<std::string> seeds;
    seeds.reserve(8);
    for (int i = 0; i < 8 && i < (int)table.size(); ++i)
        seeds.push_back(table[i].TeamId);
    return BuildTop8FromSeeds(seeds, tournamentId);
}

std::vector<Match>
KnockoutBracketBuilder::BuildTop8FromSeeds(const std::vector<std::string>& s,
                                           const std::string& tournamentId) {
    std::vector<Match> out;
    auto w = [](const Match& m) {
        return std::string("W(") + m.HomeTeamId + "-" + m.AwayTeamId + ")";
    };

    if (s.size() >= 8) {
        out.push_back(make_match(tournamentId, s[0], s[7], 1));
        out.push_back(make_match(tournamentId, s[3], s[4], 1));
        out.push_back(make_match(tournamentId, s[2], s[5], 1));
        out.push_back(make_match(tournamentId, s[1], s[6], 1));

        out.push_back(make_match(tournamentId, w(out[0]), w(out[1]), 2));
        out.push_back(make_match(tournamentId, w(out[2]), w(out[3]), 2));

        out.push_back(make_match(tournamentId, w(out[4]), w(out[5]), 3));
    } else if (s.size() >= 4) {
        out.push_back(make_match(tournamentId, s[0], s[3], 1));
        out.push_back(make_match(tournamentId, s[1], s[2], 1));

        out.push_back(make_match(tournamentId, w(out[0]), w(out[1]), 2));
    }

    return out;
}
