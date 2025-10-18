#ifndef TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
#define TOURNAMENTS_TOURNAMENTREPOSITORY_HPP

#include <string>
#include <memory>
#include <vector>

#include "persistence/repository/ITournamentRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "domain/Tournament.hpp"

class TournamentRepository : public ITournamentRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    explicit TournamentRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider);

    std::shared_ptr<domain::Tournament> ReadById(std::string id) override;
    std::string Create (const domain::Tournament & entity) override;
    std::string Update (const domain::Tournament & entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
};

#endif // TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
