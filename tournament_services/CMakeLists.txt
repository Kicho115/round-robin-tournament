project(tournament_services)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SERVICES_SOURCES
    src/delegate/TeamDelegate.cpp
    src/delegate/TournamentDelegate.cpp
    src/controller/TournamentController.cpp
    src/controller/TeamController.cpp
)

include(CTest)
enable_testing()

# --- Dependencias proporcionadas por la raíz o por paquetes ---
# Crow
if (NOT TARGET Crow::Crow)
  find_package(Crow CONFIG REQUIRED)
endif()

# nlohmann_json
if (NOT TARGET nlohmann_json::nlohmann_json)
  find_package(nlohmann_json CONFIG REQUIRED)
endif()

# libpqxx (puede venir como libpqxx::pqxx o pqxx::pqxx)
set(PQXX_TARGET "")
if (TARGET libpqxx::pqxx)
  set(PQXX_TARGET libpqxx::pqxx)
elseif (TARGET pqxx::pqxx)
  set(PQXX_TARGET pqxx::pqxx)
else()
  find_package(libpqxx CONFIG REQUIRED)
  set(PQXX_TARGET libpqxx::pqxx)
endif()

# ---------- Hypodermic (header-only) ----------
# 1) intenta por variable/paquete
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")

# 2) si no existe, tráelo con FetchContent
if (NOT HYPODERMIC_INCLUDE_DIRS)
  include(FetchContent)
  FetchContent_Declare(
    hypodermic
    GIT_REPOSITORY https://github.com/ybainier/Hypodermic.git
    GIT_TAG master
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(hypodermic)
  # ajusta el include dir
  set(HYPODERMIC_INCLUDE_DIRS "${hypodermic_SOURCE_DIR}/include" CACHE PATH "" FORCE)
  message(STATUS "Hypodermic no encontrado en el sistema; usando FetchContent -> ${HYPODERMIC_INCLUDE_DIRS}")
endif()

# Tests del subproyecto
add_subdirectory(tests)

# Binario principal del servicio
add_executable(${PROJECT_NAME}
    main.cpp
    ${SERVICES_SOURCES}
)

# Includes del propio proyecto + Hypodermic
target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${HYPODERMIC_INCLUDE_DIRS}
)

# Enlaces
target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Crow::Crow
      ${PQXX_TARGET}
      nlohmann_json::nlohmann_json
      tournament_common
)

# Copia del configuration.json al directorio de build
configure_file(
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json
    COPYONLY
)
