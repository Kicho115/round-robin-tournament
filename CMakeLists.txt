cmake_minimum_required(VERSION 3.31.6)
project(Tournaments)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ===================== Crow (sin Internet) + Asio del sistema =====================
find_package(Crow CONFIG QUIET)
if (NOT Crow_FOUND)
  # Ya tenemos Asio del sistema
  if (NOT DEFINED ASIO_INCLUDE_DIR OR NOT EXISTS "${ASIO_INCLUDE_DIR}")
    message(FATAL_ERROR "ASIO_INCLUDE_DIR no establecido. Pasa -DASIO_INCLUDE_DIR=/usr/include (o donde esté asio).")
  endif()

  # Opción A: usar headers locales de Crow (vendor) si el usuario los pasa
  if (DEFINED CROW_INCLUDE_DIR AND EXISTS "${CROW_INCLUDE_DIR}/crow.h")
    message(STATUS "Usando Crow desde headers locales: ${CROW_INCLUDE_DIR}")
    add_library(Crow::Crow INTERFACE IMPORTED)
    set_target_properties(Crow::Crow PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CROW_INCLUDE_DIR};${ASIO_INCLUDE_DIR}"
      INTERFACE_COMPILE_DEFINITIONS "ASIO_STANDALONE;CROW_USE_ASIO"
    )
  else()
    # Opción B: intentar FetchContent (requiere Internet)
    message(STATUS "Crow no encontrado como paquete; intentando FetchContent (requiere Internet)")
    # Si no hay Internet fallará aquí; si la hay, desactivamos ejemplos/tests de Crow
    set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    # Asio por FetchContent solo si no hay del sistema (pero aquí SÍ hay)
    # => no declaramos asio

    FetchContent_Declare(
      crow
      GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
      GIT_TAG v1.1.0
      GIT_SHALLOW TRUE
    )
    # Informar a Crow que usamos standalone Asio
    set(ASIO_INCLUDE_DIR "${ASIO_INCLUDE_DIR}" CACHE PATH "" FORCE)
    set(CROW_USE_ASIO ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(crow)
  endif()
else()
  message(STATUS "Crow encontrado como paquete")
endif()

# ===================== libpqxx (fallback) + PostgreSQL =====================
find_package(libpqxx CONFIG QUIET)
if (NOT libpqxx_FOUND)
  find_package(PostgreSQL REQUIRED)  # target PostgreSQL::PostgreSQL

  FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 7.9.2
    GIT_SHALLOW TRUE
    CMAKE_ARGS
      -DBUILD_TESTING=OFF
      -DPQXX_BUILD_TESTS=OFF
      -DPQXX_ENABLE_TEST=OFF
      -DPQXX_WITH_TESTS=OFF
      -DPQXX_BUILD_EXAMPLES=OFF
      -DPQXX_WITH_EXAMPLES=OFF
  )
  FetchContent_MakeAvailable(libpqxx)

  if (TARGET pqxx AND NOT TARGET pqxx::pqxx)
    add_library(pqxx::pqxx ALIAS pqxx)
  endif()
  if (TARGET pqxx)
    target_link_libraries(pqxx PUBLIC PostgreSQL::PostgreSQL)
  endif()

  message(STATUS "libpqxx no encontrado como paquete; usando FetchContent + PostgreSQL::PostgreSQL (sin tests)")
else()
  message(STATUS "libpqxx encontrado como paquete")
endif()

# ===================== nlohmann_json (fallback) =====================
find_package(nlohmann_json CONFIG QUIET)
if (NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# ===================== activemq-cpp (OPCIONAL) =====================
set(BUILD_TOURNAMENT_CONSUMER ON CACHE BOOL "Build tournament_consumer binary")
find_package(activemq-cpp CONFIG QUIET)
if (NOT activemq-cpp_FOUND)
  message(WARNING "activemq-cpp no encontrado; se omitirá 'tournament_consumer'. "
                  "Para habilitarlo instala activemq-cpp y reconfigura.")
  set(BUILD_TOURNAMENT_CONSUMER OFF CACHE BOOL "Build tournament_consumer binary" FORCE)
endif()

# ===================== Subproyectos =====================
add_subdirectory(tournament_common)
add_subdirectory(tournament_services)
if (BUILD_TOURNAMENT_CONSUMER)
  add_subdirectory(tournament_consumer)
endif()
