#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>

#include "domain/Tournament.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"

// Mock for IRepository<domain::Tournament, std::string>
class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string> {
public:
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
};

// Mock for IQueueMessageProducer
class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
};

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<QueueMessageProducerMock> queueMessageProducerMock;
    std::shared_ptr<TournamentDelegate> tournamentDelegate;

    // Before each test
    void SetUp() override {
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        queueMessageProducerMock = std::make_shared<QueueMessageProducerMock>();
        tournamentDelegate = std::make_shared<TournamentDelegate>(
            tournamentRepositoryMock, 
            queueMessageProducerMock
        );
    }

    // After each test
    void TearDown() override {
    }
};

// Test 1: CreateTournament - Validate successful insertion and returned ID
TEST_F(TournamentDelegateTest, CreateTournament_ValidInsertion_ReturnsGeneratedId) {
    // Arrange
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", domain::TournamentFormat(2, 8));
    domain::Tournament capturedTournament;
    std::string expectedId = "generated-tournament-id-123";

    EXPECT_CALL(*tournamentRepositoryMock, Create(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTournament),
            testing::Return(expectedId)
        ));
    
    EXPECT_CALL(*queueMessageProducerMock, SendMessage(expectedId, "tournament.created"))
        .Times(1);

    // Act
    auto result = tournamentDelegate->CreateTournament(tournament);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), expectedId);
    EXPECT_EQ(capturedTournament.Name(), "Test Tournament");
    EXPECT_EQ(capturedTournament.Format().NumberOfGroups(), 2);
    EXPECT_EQ(capturedTournament.Format().MaxTeamsPerGroup(), 8);
}

// Test 2: CreateTournament - Validate failed insertion and error message
TEST_F(TournamentDelegateTest, CreateTournament_FailedInsertion_ReturnsError) {
    // Arrange
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", domain::TournamentFormat());
    
    EXPECT_CALL(*tournamentRepositoryMock, Create(testing::_))
        .WillOnce(testing::Return("")); // Empty string simulates failure

    // Act
    auto result = tournamentDelegate->CreateTournament(tournament);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Failed to create tournament");
}

// Test 3: ReadById - Validate successful read with valid object
TEST_F(TournamentDelegateTest, ReadById_ValidId_ReturnsValidTournamentObject) {
    // Arrange
    std::string tournamentId = "tournament-id-456";
    auto expectedTournament = std::make_shared<domain::Tournament>("Championship Tournament", domain::TournamentFormat(4, 16));
    expectedTournament->Id() = tournamentId;

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq(tournamentId)))
        .WillOnce(testing::Return(expectedTournament));

    // Act
    auto result = tournamentDelegate->ReadById(tournamentId);

    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Id(), tournamentId);
    EXPECT_EQ(result->Name(), "Championship Tournament");
    EXPECT_EQ(result->Format().NumberOfGroups(), 4);
    EXPECT_EQ(result->Format().MaxTeamsPerGroup(), 16);
}

// Test 4: ReadById - Validate null result for non-existent tournament
TEST_F(TournamentDelegateTest, ReadById_InvalidId_ReturnsNullptr) {
    // Arrange
    std::string tournamentId = "non-existent-id";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq(tournamentId)))
        .WillOnce(testing::Return(nullptr));

    // Act
    auto result = tournamentDelegate->ReadById(tournamentId);

    // Assert
    EXPECT_EQ(result, nullptr);
}

// Test 5: ReadAll - Validate result with a list of tournament objects
TEST_F(TournamentDelegateTest, ReadAll_MultipleObjects_ReturnsListOfTournaments) {
    // Arrange
    auto tournament1 = std::make_shared<domain::Tournament>("Tournament 1", domain::TournamentFormat(2, 8));
    tournament1->Id() = "id-1";
    
    auto tournament2 = std::make_shared<domain::Tournament>("Tournament 2", domain::TournamentFormat(3, 12));
    tournament2->Id() = "id-2";
    
    auto tournament3 = std::make_shared<domain::Tournament>("Tournament 3", domain::TournamentFormat(4, 16));
    tournament3->Id() = "id-3";

    std::vector<std::shared_ptr<domain::Tournament>> expectedTournaments = {
        tournament1, tournament2, tournament3
    };

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(expectedTournaments));

    // Act
    auto result = tournamentDelegate->ReadAll();

    // Assert
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]->Id(), "id-1");
    EXPECT_EQ(result[0]->Name(), "Tournament 1");
    EXPECT_EQ(result[1]->Id(), "id-2");
    EXPECT_EQ(result[1]->Name(), "Tournament 2");
    EXPECT_EQ(result[2]->Id(), "id-3");
    EXPECT_EQ(result[2]->Name(), "Tournament 3");
}

// Test 6: ReadAll - Validate empty list result
TEST_F(TournamentDelegateTest, ReadAll_EmptyRepository_ReturnsEmptyList) {
    // Arrange
    std::vector<std::shared_ptr<domain::Tournament>> emptyList;

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    // Act
    auto result = tournamentDelegate->ReadAll();

    // Assert
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.size(), 0);
}

// Test 7: UpdateTournament - Validate successful update with ID search
TEST_F(TournamentDelegateTest, UpdateTournament_ValidUpdate_ReturnsSuccessfulUpdate) {
    // Arrange
    std::string tournamentId = "tournament-id-789";
    auto existingTournament = std::make_shared<domain::Tournament>("Old Name", domain::TournamentFormat(2, 8));
    existingTournament->Id() = tournamentId;
    
    auto updatedTournament = std::make_shared<domain::Tournament>("Updated Name", domain::TournamentFormat(3, 12));
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq(tournamentId)))
        .WillOnce(testing::Return(existingTournament));
    
    EXPECT_CALL(*tournamentRepositoryMock, Update(testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTournament),
            testing::Return(tournamentId)
        ));

    // Act
    auto result = tournamentDelegate->UpdateTournament(tournamentId, updatedTournament);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), tournamentId);
    EXPECT_EQ(capturedTournament.Id(), tournamentId);
    EXPECT_EQ(capturedTournament.Name(), "Updated Name");
    EXPECT_EQ(capturedTournament.Format().NumberOfGroups(), 3);
    EXPECT_EQ(capturedTournament.Format().MaxTeamsPerGroup(), 12);
}

// Test 8: UpdateTournament - Validate failed search and error message
TEST_F(TournamentDelegateTest, UpdateTournament_TournamentNotFound_ReturnsError) {
    // Arrange
    std::string tournamentId = "non-existent-tournament-id";
    auto updatedTournament = std::make_shared<domain::Tournament>("Updated Name", domain::TournamentFormat(3, 12));

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq(tournamentId)))
        .WillOnce(testing::Return(nullptr));

    // No Update call should be made since tournament doesn't exist
    EXPECT_CALL(*tournamentRepositoryMock, Update(testing::_))
        .Times(0);

    // Act
    auto result = tournamentDelegate->UpdateTournament(tournamentId, updatedTournament);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Tournament not found");
}

