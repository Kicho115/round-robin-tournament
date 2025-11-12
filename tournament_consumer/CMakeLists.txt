project(tournament_consumer)

set(CMAKE_CXX_STANDARD 23)
set(CONSUMER_SOURCES
        src/consumer/MatchGenerationConsumer.cpp
        src/consumer/ScoreProcessingConsumer.cpp
)

include(CTest)
enable_testing()

find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(include)

add_executable(${PROJECT_NAME}
        main.cpp
        ${CONSUMER_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        tournament_common
        -Wl,--start-group
        /opt/vcpkg/installed/x64-linux/debug/lib/libpq.a
        /opt/vcpkg/installed/x64-linux/debug/lib/libpgcommon.a
        /opt/vcpkg/installed/x64-linux/debug/lib/libpgport.a
        -Wl,--end-group
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_include_directories(${PROJECT_NAME} PRIVATE ${HYPODERMIC_INCLUDE_DIRS})

# Tests
add_subdirectory(tests)

configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json   # source file
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json  # destination
        COPYONLY
)