project(tournament_services)

set(CMAKE_CXX_STANDARD 23)
set(SERVICES_SOURCES
        src/delegate/TeamDelegate.cpp
        src/delegate/TournamentDelegate.cpp
        src/controller/TournamentController.cpp
        src/controller/TeamController.cpp)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)


add_subdirectory(tests)

include_directories(include)

add_executable(${PROJECT_NAME}
        main.cpp
        ${SERVICES_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        tournament_common
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        -Wl,--start-group
        /opt/vcpkg/installed/x64-linux/debug/lib/libpq.a
        /opt/vcpkg/installed/x64-linux/debug/lib/libpgcommon.a
        /opt/vcpkg/installed/x64-linux/debug/lib/libpgport.a
        -Wl,--end-group
        OpenSSL::SSL
        OpenSSL::Crypto)

target_include_directories(${PROJECT_NAME} INTERFACE ${HYPODERMIC_INCLUDE_DIRS})

configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json   # source file
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json  # destination
        COPYONLY)