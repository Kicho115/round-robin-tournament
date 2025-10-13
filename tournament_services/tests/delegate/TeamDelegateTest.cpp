#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "delegate/TeamDelegate.hpp"
#include "persistence/repository/IRepository.hpp"

class TeamRepoMock : public IRepository<domain::Team, std::string_view> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string_view), (override));
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
    EXPECT_EQ("team_not_found", r.value());
}

TEST(TeamDelegate, SaveTeam_Fails_ReturnsError) {
    auto repo = std::make_shared<TeamRepoMock>();
    EXPECT_CALL(*repo, Create(::testing::_))
        .WillOnce(::testing::Throw(std::runtime_error("insert_error")));
    TeamDelegate sut(repo);
    domain::Team t{"","A"};
    try {
        auto id = sut.SaveTeam(t);
        FAIL() << "Expected exception";
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "insert_error");
    }
}

TEST(TeamDelegate, GetTeam_ReturnsObject) {
    auto repo = std::make_shared<TeamRepoMock>();
    auto team = std::make_shared<domain::Team>(domain::Team{"id1","A"});
    EXPECT_CALL(*repo, ReadById("id1"))
        .WillOnce(::testing::Return(team));
    TeamDelegate sut(repo);
    auto result = sut.GetTeam("id1");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Id, "id1");
    EXPECT_EQ(result->Name, "A");
}

TEST(TeamDelegate, GetTeam_ReturnsNullptr) {
    auto repo = std::make_shared<TeamRepoMock>();
    EXPECT_CALL(*repo, ReadById("id2"))
        .WillOnce(::testing::Return(nullptr));
    TeamDelegate sut(repo);
    auto result = sut.GetTeam("id2");
    EXPECT_EQ(result, nullptr);
}

TEST(TeamDelegate, GetAllTeams_ReturnsList) {
    auto repo = std::make_shared<TeamRepoMock>();
    std::vector<std::shared_ptr<domain::Team>> teams = {
        std::make_shared<domain::Team>(domain::Team{"id1","A"}),
        std::make_shared<domain::Team>(domain::Team{"id2","B"})
    };
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(::testing::Return(teams));
    TeamDelegate sut(repo);
    auto result = sut.GetAllTeams();
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0]->Id, "id1");
    EXPECT_EQ(result[1]->Id, "id2");
}

TEST(TeamDelegate, GetAllTeams_ReturnsEmptyList) {
    auto repo = std::make_shared<TeamRepoMock>();
    EXPECT_CALL(*repo, ReadAll())
        .WillOnce(::testing::Return(std::vector<std::shared_ptr<domain::Team>>{}));
    TeamDelegate sut(repo);
    auto result = sut.GetAllTeams();
    EXPECT_TRUE(result.empty());
}

TEST(TeamDelegate, UpdateTeam_Success) {
    auto repo = std::make_shared<TeamRepoMock>();
    auto team = std::make_shared<domain::Team>(domain::Team{"id1","A"});
    EXPECT_CALL(*repo, ReadById("id1"))
        .WillOnce(::testing::Return(team));
    EXPECT_CALL(*repo, Update(::testing::_))
        .WillOnce(::testing::Return("id1"));
    TeamDelegate sut(repo);
    domain::Team t{"id1","A"};
    auto r = sut.UpdateTeam(t);
    EXPECT_FALSE(r.has_value());
}
