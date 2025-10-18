#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>
#include <cstdint>
#include <utility>
#include <nlohmann/json.hpp>

namespace domain {

enum class MatchPhase : std::uint8_t {
    RoundRobin = 0,
    Knockout   = 1
};

inline std::string ToString(MatchPhase p) {
    switch (p) {
        case MatchPhase::RoundRobin: return "RR";
        case MatchPhase::Knockout:   return "KO";
    }
    return "RR";
}

inline MatchPhase PhaseFromString(const std::string& s) {
    if (s == "KO" || s == "knockout" || s == "Knockout") return MatchPhase::Knockout;
    return MatchPhase::RoundRobin;
}

struct Match {
    // Identificadores
    std::string Id;            // puede estar vacío si aún no se persiste
    std::string TournamentId;  // requerido
    std::string GroupId;       // vacío en KO si no aplica

    // Equipos
    std::string HomeTeamId;
    std::string AwayTeamId;

    // Metadata del scheduling
    MatchPhase  Phase { MatchPhase::RoundRobin };
    int         Round { 0 };   // Para RR: jornada; para KO: ronda (1=QF,2=SF,3=Final)

    // Marcador (opcional hasta que se reporte)
    std::optional<int> HomeScore;
    std::optional<int> AwayScore;

    Match() = default;

    Match(std::string tournamentId,
          std::string groupId,
          std::string homeId,
          std::string awayId,
          MatchPhase  phase,
          int         round,
          std::string id = {})
        : Id(std::move(id)),
          TournamentId(std::move(tournamentId)),
          GroupId(std::move(groupId)),
          HomeTeamId(std::move(homeId)),
          AwayTeamId(std::move(awayId)),
          Phase(phase),
          Round(round) {}

    bool IsScored() const {
        return HomeScore.has_value() && AwayScore.has_value();
    }

    void SetScore(int home, int away) {
        HomeScore = home;
        AwayScore = away;
    }
};

// JSON (nlohmann) -------------------------------------------------------------

inline void to_json(nlohmann::json& j, const Match& m) {
    j = nlohmann::json{
        {"id",            m.Id},
        {"tournamentId",  m.TournamentId},
        {"groupId",       m.GroupId},
        {"homeTeamId",    m.HomeTeamId},
        {"awayTeamId",    m.AwayTeamId},
        {"phase",         ToString(m.Phase)},
        {"round",         m.Round}
    };

    if (m.HomeScore.has_value()) j["homeScore"] = *m.HomeScore;
    if (m.AwayScore.has_value()) j["awayScore"] = *m.AwayScore;
}

inline void from_json(const nlohmann::json& j, Match& m) {
    m.Id           = j.value("id", std::string{});
    m.TournamentId = j.value("tournamentId", std::string{});
    m.GroupId      = j.value("groupId", std::string{});
    m.HomeTeamId   = j.value("homeTeamId", std::string{});
    m.AwayTeamId   = j.value("awayTeamId", std::string{});
    m.Phase        = PhaseFromString(j.value("phase", std::string{"RR"}));
    m.Round        = j.value("round", 0);

    if (j.contains("homeScore") && !j["homeScore"].is_null())
        m.HomeScore = j["homeScore"].get<int>();
    else
        m.HomeScore.reset();

    if (j.contains("awayScore") && !j["awayScore"].is_null())
        m.AwayScore = j["awayScore"].get<int>();
    else
        m.AwayScore.reset();
}

} // namespace domain

#endif // DOMAIN_MATCH_HPP
