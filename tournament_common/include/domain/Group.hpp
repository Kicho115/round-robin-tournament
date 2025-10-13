#ifndef DOMAIN_GROUP_HPP
#define DOMAIN_GROUP_HPP

#include <string>
#include <vector>

#include "domain/Team.hpp"

namespace domain {
    class Group {
        /* data */
        std::string id;
        std::string name;
        std::string tournamentId;
        std::vector<Team> teams;

    public:
        explicit Group(const std::string_view & name = "", const std::string_view&  id = "") : id(id), name(name) {
        }

        [[nodiscard]] std::string Id() const {
            return  id;
        }

        std::string& Id() {
            return  id;
        }

        [[nodiscard]] std::string Name() const {
            return  name;
        }

        [[nodiscard]] std::string & Name() {
            return  name;
        }

        [[nodiscard]] std::string TournamentId() const {
            return  tournamentId;
        }

        [[nodiscard]] std::string & TournamentId() {
            return  tournamentId;
        }

        [[nodiscard]] std::vector<Team> Teams() const {
            return this->teams;
        }

        [[nodiscard]] std::vector<Team> & Teams() {
            return this->teams;
        }
    };

    inline bool operator==(const Team& lhs, const Team& rhs) {
        return lhs.Id == rhs.Id && lhs.Name == rhs.Name;
    }

    inline bool operator==(const Group& lhs, const Group& rhs) {
        return lhs.Id() == rhs.Id() && lhs.Name() == rhs.Name() &&
               lhs.TournamentId() == rhs.TournamentId() && lhs.Teams() == rhs.Teams();
    }
}

#endif