cmake_minimum_required(VERSION 3.16)
project(tournament_tests)

set(TEST_SOURCES
        controller/GroupControllerTest.cpp
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/EventingGroupDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/RoundRobinMatchGeneratorTest.cpp
        delegate/TeamAddedConsumerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        delegate/RRGenerator_ThirtyTwoTeams_Test.cpp
        delegate/StandingsCalculatorTest.cpp
        delegate/KnockoutBracketBuilderTest.cpp
        ../src/controller/GroupController.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/controller/MatchController.cpp
        ../src/delegate/EventingGroupDelegate.cpp
        ../src/delegate/GroupDelegate.cpp
        ../src/delegate/RoundRobinGenerator.cpp
        ../src/delegate/RoundRobinMatchGenerator.cpp
        ../src/delegate/TeamAddedConsumer.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/MatchDelegate.cpp
)



set(SOURCES ${TEST_SOURCES})
include_directories(../include)

# Usar el toolchain de vcpkg si está disponible
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Usando toolchain de vcpkg: ${CMAKE_TOOLCHAIN_FILE}")
endif()

find_package(GTest CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

# Enlazar todas las dependencias necesarias
# tournament_common debe estar correctamente exportado en el proyecto principal
# Si no, agregar la ruta a la librería manualmente

# Enlaza Crow, nlohmann_json, libpqxx, GTest y tournament_common
# Si alguna dependencia no se encuentra, CMake fallará y mostrará el error

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    Crow::Crow
    nlohmann_json::nlohmann_json
    libpqxx::pqxx
    tournament_common
)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)