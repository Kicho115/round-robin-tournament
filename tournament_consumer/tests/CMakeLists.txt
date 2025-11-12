cmake_minimum_required(VERSION 3.16)
project(tournament_consumer_tests)

set(TEST_SOURCES
        MatchGenerationConsumerTest.cpp
        ScoreProcessingConsumerTest.cpp
        ../src/consumer/MatchGenerationConsumer.cpp
        ../src/consumer/ScoreProcessingConsumer.cpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(../include)

find_package(GTest CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    nlohmann_json::nlohmann_json
    libpqxx::pqxx
    tournament_common
)

add_test(AllConsumerTests ${PROJECT_NAME}_runner)


