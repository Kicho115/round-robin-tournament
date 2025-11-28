#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "domain/Tournament.hpp"
#include "scheduling/IMatchScheduler.hpp"

struct TeamAddedEvent {
    std::string tournamentId;
    std::string groupId;
    std::string teamId;
    std::string teamName;

    static TeamAddedEvent FromJson(const nlohmann::json& j) {
        TeamAddedEvent e;
        e.tournamentId = j.value("tournamentId", "");
        e.groupId      = j.value("groupId", "");
        if (j.contains("team") && j["team"].is_object()) {
            e.teamId   = j["team"].value("id", "");
            e.teamName = j["team"].value("name", "");
        } else {
            e.teamId   = j.value("teamId", "");
            e.teamName = j.value("teamName", "");
        }
        return e;
    }
};

class TeamAddedConsumer {
    std::shared_ptr<IGroupRepository> groupRepo;
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo;
    std::shared_ptr<IMatchScheduler> scheduler;

public:
    TeamAddedConsumer(std::shared_ptr<IGroupRepository> groupRepo_,
                      std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo_,
                      std::shared_ptr<IMatchScheduler> scheduler_)
        : groupRepo(std::move(groupRepo_)),
          tournamentRepo(std::move(tournamentRepo_)),
          scheduler(std::move(scheduler_)) {}

    void Handle(const nlohmann::json& eventJson);
};
