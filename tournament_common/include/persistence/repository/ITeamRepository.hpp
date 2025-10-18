#pragma once
#include <memory>
#include <string>
#include "IRepository.hpp"
#include "domain/Team.hpp"

class ITeamRepository : public IRepository<domain::Team, std::string> {
public:
    ~ITeamRepository() override = default;
    std::shared_ptr<domain::Team> ReadById(std::string id) override = 0;
};
