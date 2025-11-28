//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>
#include <expected>
#include <format>

#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Group.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string> > repository,
                                       std::shared_ptr<IGroupRepository> groupRepository,
                                       std::shared_ptr<IQueueMessageProducer> producer) 
    : tournamentRepository(std::move(repository)),
      groupRepository(std::move(groupRepository)),
      producer(std::move(producer)) {
}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    try {
        std::shared_ptr<domain::Tournament> tp = std::move(tournament);

        // Create the tournament first
        std::string id = tournamentRepository->Create(*tp);
        if (id.empty()) {
            return std::unexpected("Failed to create tournament");
        }
        producer->SendMessage(id, "tournament.created");

        // For round-robin tournaments, create a single group as a team container
        // This group is used to hold all teams, and matches will be generated
        // automatically when all teams are added
        domain::Group group;
        group.Name() = "All Teams";
        group.TournamentId() = id;
        
        std::string groupId = groupRepository->Create(group);
        if (groupId.empty()) {
            return std::unexpected("Failed to create teams container");
        }

        return id;
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error creating tournament: ") + e.what());
    }
}

std::vector<std::shared_ptr<domain::Tournament> > TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

std::shared_ptr<domain::Tournament> TournamentDelegate::ReadById(const std::string& id) {
    return tournamentRepository->ReadById(id);
}

std::expected<std::string, std::string> TournamentDelegate::UpdateTournament(const std::string& id, std::shared_ptr<domain::Tournament> tournament) {
    try {
        auto existingTournament = tournamentRepository->ReadById(id);
        if (!existingTournament) {
            return std::unexpected("Tournament not found");
        }
        
        tournament->Id() = id;
        std::string updatedId = tournamentRepository->Update(*tournament);
        if (updatedId.empty()) {
            return std::unexpected("Failed to update tournament");
        }
        return updatedId;
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error updating tournament: ") + e.what());
    }
}