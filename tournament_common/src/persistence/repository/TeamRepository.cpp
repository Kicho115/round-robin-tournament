//
// Created by ricampos on 10/12/25.
//

#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace domain;

class TeamRepository : public IRepository<Team, std::string_view> {
public:
    TeamRepository() = default;
    std::shared_ptr<Team> ReadById(std::string_view id) override {
        auto it = teams.find(std::string(id));
        if (it != teams.end()) {
            return std::make_shared<Team>(it->second);
        }
        return nullptr;
    }
    std::string_view Create(const Team& team) override {
        teams[team.Id] = team;
        return team.Id;
    }
    std::string_view Update(const Team& team) override {
        teams[team.Id] = team;
        return team.Id;
    }
    void Delete(std::string_view id) override {
        teams.erase(std::string(id));
    }
    std::vector<std::shared_ptr<Team>> ReadAll() override {
        std::vector<std::shared_ptr<Team>> result;
        for (const auto& kv : teams) {
            result.push_back(std::make_shared<Team>(kv.second));
        }
        return result;
    }
private:
    std::map<std::string, Team> teams;
};
