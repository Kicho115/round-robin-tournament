#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "delegate/TeamDelegate.hpp"
#include "persistence/repository/IRepository.hpp"

class TeamRepoMock : public IRepository<domain::Team, std::string_view> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::string, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
};

TEST(TeamDelegate, SaveTeam_CallsRepoCreate) {
    auto repo = std::make_shared<TeamRepoMock>();
    EXPECT_CALL(*repo, Create(::testing::_))
        .WillOnce(::testing::Return("new-id"));
    TeamDelegate sut(repo);
    domain::Team t{"","A"};
    auto id = sut.SaveTeam(t);
    EXPECT_EQ(id, "new-id");
}

TEST(TeamDelegate, UpdateTeam_404_WhenNotExists) {
    auto repo = std::make_shared<TeamRepoMock>();
    EXPECT_CALL(*repo, ReadById(::testing::_))
        .WillOnce(::testing::Return(nullptr));
    TeamDelegate sut(repo);
    domain::Team t{"id-x","A"};
    auto r = sut.UpdateTeam(t);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ("team_not_found", r.error());
}
