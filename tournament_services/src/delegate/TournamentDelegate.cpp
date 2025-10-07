//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>
#include <expected>

#include "delegate/TournamentDelegate.hpp"

#include "persistence/repository/IRepository.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string> > repository, std::shared_ptr<IQueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer)) {
}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    try {
        //fill groups according to max groups
        std::shared_ptr<domain::Tournament> tp = std::move(tournament);
        // for (auto[i, g] = std::tuple{0, 'A'}; i < tp->Format().NumberOfGroups(); i++,g++) {
        //     tp->Groups().push_back(domain::Group{std::format("Tournament {}", g)});
        // }

        std::string id = tournamentRepository->Create(*tp);
        if (id.empty()) {
            return std::unexpected("Failed to create tournament");
        }
        producer->SendMessage(id, "tournament.created");

        //if groups are completed also create matches

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