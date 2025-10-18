#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/IGroupRepository.hpp"
#include "delegate/IEventPublisher.hpp"

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
    std::shared_ptr<IEventPublisher> publisher;
    int maxTeamsPerGroup;
    std::string topic;

public:
    TeamAddedConsumer(std::shared_ptr<IGroupRepository> groupRepo_,
                      std::shared_ptr<IEventPublisher> publisher_,
                      int maxTeamsPerGroup_,
                      std::string topic_ = "matches.created")
        : groupRepo(std::move(groupRepo_)),
          publisher(std::move(publisher_)),
          maxTeamsPerGroup(maxTeamsPerGroup_),
          topic(std::move(topic_)) {}

    void Handle(const nlohmann::json& eventJson);
    void HandleTeamAdded(const TeamAddedEvent& e);
    void HandleTeamAdded(const std::string& tournamentId, const std::string& groupId);
};
