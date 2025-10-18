#pragma once
#include <memory>
#include <string>
#include "IRepository.hpp"
#include "domain/Tournament.hpp"

class ITournamentRepository : public IRepository<domain::Tournament, std::string> {
public:
    ~ITournamentRepository() override = default;
    // Garantizamos que ReadById esté expuesto
    std::shared_ptr<domain::Tournament> ReadById(std::string id) override = 0;
};
