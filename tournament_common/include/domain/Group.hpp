// tournament_common/include/domain/Group.hpp
#ifndef DOMAIN_GROUP_HPP
#define DOMAIN_GROUP_HPP

#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

#include "domain/Team.hpp"

namespace domain {
    class Group {
        std::string id;
        std::string name;
        std::string tournamentId;
        std::vector<Team> teams;

    public:
        explicit Group(std::string_view id_ = "", std::string_view name_ = "")
          : id(id_), name(name_) {}

        // ctor desde JSON (usado por el repositorio)
        explicit Group(const nlohmann::json& j) {
            if (j.contains("id"))           id = j.at("id").get<std::string>();
            if (j.contains("name"))         name = j.at("name").get<std::string>();
            if (j.contains("tournamentId")) tournamentId = j.at("tournamentId").get<std::string>();
            if (j.contains("teams") && j["teams"].is_array()) {
                teams.clear();
                teams.reserve(j["teams"].size());
                for (const auto& jt : j["teams"]) {
                    Team t{};
                    if (jt.contains("id"))   t.Id   = jt.at("id").get<std::string>();
                    if (jt.contains("name")) t.Name = jt.at("name").get<std::string>();
                    teams.push_back(std::move(t));
                }
            }
        }

        [[nodiscard]] std::string Id() const { return id; }
        std::string& Id() { return id; }

        [[nodiscard]] std::string Name() const { return name; }
        std::string& Name() { return name; }

        [[nodiscard]] std::string TournamentId() const { return tournamentId; }
        std::string& TournamentId() { return tournamentId; }

        [[nodiscard]] std::vector<Team> Teams() const { return teams; }
        std::vector<Team>& Teams() { return teams; }
    };

    inline bool operator==(const Team& lhs, const Team& rhs) {
        return lhs.Id == rhs.Id && lhs.Name == rhs.Name;
    }

    inline bool operator==(const Group& lhs, const Group& rhs) {
        return lhs.Id() == rhs.Id()
            && lhs.Name() == rhs.Name()
            && lhs.TournamentId() == rhs.TournamentId()
            && lhs.Teams() == rhs.Teams();
    }
} // namespace domain

#endif // DOMAIN_GROUP_HPP
