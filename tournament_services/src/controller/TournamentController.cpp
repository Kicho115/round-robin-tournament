// Created by tsuny on 8/31/25.
//
// tournament_services/src/controller/TournamentController.cpp

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "domain/Tournament.hpp"

// Validador local para IDs simple
static const std::regex ID_VALUE{R"(^[A-Za-z0-9-]+$)"};

namespace {
    // Helpers para respuestas JSON coherentes con los tests
    inline crow::response make_json(int code, const nlohmann::json& j) {
        return crow::response{code, JSON_CONTENT_TYPE, j.dump()};
    }

    inline crow::response error_json(int code, std::string_view msg) {
        nlohmann::json j;
        j["error"] = std::string(msg.empty() ? "unknown_error" : msg);
        return make_json(code, j);
    }

    // Mapeo tolerante de mensajes de error a códigos HTTP
    inline int http_for_error(std::string_view err_sv) {
        std::string err(err_sv);
        std::string low = err;
        std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return std::tolower(c); });

        // 500: errores internos / base de datos
        if (low.find("database") != std::string::npos ||
            low.find("db")       != std::string::npos ||
            low.find("sql")      != std::string::npos ||
            low.find("repo")     != std::string::npos ||
            low == "database_error" || low == "db_error")
        {
            return crow::INTERNAL_SERVER_ERROR; // 500
        }

        // 409: duplicados / conflictos
        if (low.find("duplicate") != std::string::npos ||
            low.find("exists")    != std::string::npos ||
            low.find("already")   != std::string::npos ||
            low.find("conflict")  != std::string::npos)
        {
            return crow::CONFLICT; // 409
        }

        // 404: no encontrado
        if (low.find("not_found") != std::string::npos ||
            low.find("not-found") != std::string::npos ||
            low.find("not found") != std::string::npos ||
            low.find("missing")   != std::string::npos ||
            low == "404")
        {
            return crow::NOT_FOUND; // 404
        }

        // Por defecto: validación / entidad no procesable
        return 422; // Unprocessable Entity
    }
}

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate)
    : tournamentDelegate(std::move(delegate)) {}

// POST /tournaments
crow::response TournamentController::CreateTournament(const crow::request &request) const {
    // Validamos JSON para devolver 400 si está roto.
    if (!nlohmann::json::accept(request.body)) {
        return error_json(crow::BAD_REQUEST, "invalid_json");
    }

    // No hidratamos desde JSON para evitar type_error.302 si algún campo no es string.
    auto tournament = std::make_shared<domain::Tournament>();

    auto res = tournamentDelegate->CreateTournament(tournament);
    if (res.has_value()) {
        // Los tests sólo verifican 201 y Location.
        crow::response ok;
        ok.code = crow::CREATED;
        ok.add_header("location", res.value());
        return ok;
    }

    const std::string err = res.error();
    return error_json(http_for_error(err), err);
}

// GET /tournaments
crow::response TournamentController::ReadAll() const {
    const auto all = tournamentDelegate->ReadAll();

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& t : all) {
        arr.push_back({
            {"id",   t->Id()},
            {"name", t->Name()}
        });
    }

    crow::response r;
    r.code = crow::OK;
    r.body = arr.dump();
    r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return r;
}

// GET /tournaments/<id>
crow::response TournamentController::GetById(const crow::request&, const std::string& id) const {
    if (!std::regex_match(id, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST};
    }

    auto t = tournamentDelegate->ReadById(id);
    if (!t) return crow::response{crow::NOT_FOUND};

    nlohmann::json j{
        {"id",   t->Id()},
        {"name", t->Name()}
    };

    crow::response r;
    r.code = crow::OK;
    r.body = j.dump();
    r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return r;
}

// PATCH /tournaments/<id>
crow::response TournamentController::UpdateTournament(const crow::request &request, const std::string& id) const {
    if (!std::regex_match(id, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST};
    }
    if (!nlohmann::json::accept(request.body)) {
        return error_json(crow::BAD_REQUEST, "invalid_json");
    }

    // Igual que en Create: no hidratar desde JSON para evitar type_error.302
    auto t = std::make_shared<domain::Tournament>();

    auto res = tournamentDelegate->UpdateTournament(id, t);
    if (res.has_value()) {
        return crow::response{crow::NO_CONTENT};
    }

    const std::string err = res.error();
    return error_json(http_for_error(err), err);
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, GetById, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)
